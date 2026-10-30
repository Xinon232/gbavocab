#pragma once
#include <cstdint>
#include <cstring>
#include "writer_core.h"
#include "dictionary_fold.h"
// Borrowed read caches for version-4 lookups: two 4 KiB blocks and eight
// 512-byte directory sectors. The GBA catalog lends the three inactive TXT I/O
// windows, so no permanent EWRAM is added. Tags identify source/file/offset.
struct DictionaryWorkspace {
 static constexpr unsigned BLOCKS=2,SECTORS=8;
 unsigned char* block[BLOCKS]={};unsigned char* sector=nullptr;
 bool reuse=true;
 const void* block_owner[BLOCKS]={};int block_file[BLOCKS]={};uint32_t block_at[BLOCKS]={},block_used[BLOCKS]={};
 const void* sector_owner[SECTORS]={};int sector_file[SECTORS]={};uint32_t sector_at[SECTORS]={},sector_used[SECTORS]={};
 uint32_t clock=0,misses=0;
 void invalidate(){for(auto& o:block_owner)o=nullptr;for(auto& o:sector_owner)o=nullptr;}
};
// Versioned external file reader; no allocation and no ROM payload.
struct DictionarySource {
 virtual bool read(int file,uint32_t at,void* data,unsigned n)=0;
 virtual uint32_t size(int file)=0;
#ifdef __DEVKITARM__
 virtual DictionaryWorkspace& workspace()=0;
#else
 // Host sources may change bytes between calls: scratch buffers, no reuse.
 virtual DictionaryWorkspace& workspace(){
  static unsigned char blocks[2][4096],sectors[8*512];static DictionaryWorkspace w;
  if(!w.sector){w.block[0]=blocks[0];w.block[1]=blocks[1];w.sector=sectors;w.reuse=false;}return w;
 }
#endif
protected:
 ~DictionarySource()=default;
};
namespace dict_format {
constexpr unsigned HEADER=160,SLOT=208,LIMIT=512,SECTOR=512,BLOCK=4096,DIR_ENTRIES=15,DIR_KEY=24;
constexpr uint32_t BASE_LIMIT=64u*1024*1024,MAX_ENTRIES=(1u<<24)-1;
inline uint32_t number(const void* data){auto p=static_cast<const unsigned char*>(data);return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline unsigned half(const unsigned char* p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline void put(unsigned char* p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=v>>(8*i);}
inline uint32_t crc(const void* data,unsigned n){
 static constexpr uint32_t table[16]={0x0u,0x1db71064u,0x3b6e20c8u,0x26d930acu,0x76dc4190u,0x6b6b51f4u,0x4db26158u,0x5005713cu,0xedb88320u,0xf00f9344u,0xd6d6a3e8u,0xcb61b38cu,0x9b64c2b0u,0x86d3d2d4u,0xa00ae278u,0xbdbdf21cu};
 auto p=static_cast<const unsigned char*>(data);uint32_t c=~0u;while(n--){c^=*p++;c=(c>>4)^table[c&15];c=(c>>4)^table[c&15];}return ~c;
}
// One nonblank field: strict UTF-8, no TAB/newline/controls.
inline bool field_ok(const char* p,unsigned n){if(!n||!writer::valid_utf8(p,n))return false;bool visible=false;for(unsigned i=0;i<n;++i){unsigned c=static_cast<unsigned char>(p[i]);if(c<32||c==127)return false;if(c>32)visible=true;}return visible;}
inline bool row_ok(const char* row){unsigned n=0;while(n<192&&row[n])++n;if(n>=192||!writer::valid_utf8(row,n))return false;const char* tab=std::strchr(row,'\t');if(!tab||std::strchr(tab+1,'\t'))return false;bool a=false,b=false;for(unsigned i=0;i<n;++i){unsigned c=static_cast<unsigned char>(row[i]);if((c<32&&c!=9)||c==127)return false;if(c>32)(row+i<tab?a:b)=true;}return a&&b;}
// 0 uncommitted, 1 committed/valid, -1 committed/corrupt.
inline int slot(const unsigned char* r,unsigned seq,bool v2=false){
 if(std::memcmp(r+204,"OK01",4))return 0;
 bool add=!std::memcmp(r,"ADD1",4),edit=v2&&!std::memcmp(r,"REP2",4),del=v2&&!std::memcmp(r,"DEL2",4);
 if((!add&&!edit&&!del)||(add&&number(r+4)!=seq)||crc(r,200)!=number(r+200))return -1;
 if(del){for(unsigned n=8;n<200;++n)if(r[n])return -1;return 1;}
 if(!row_ok(reinterpret_cast<const char*>(r+8)))return -1;
 unsigned n=std::strlen(reinterpret_cast<const char*>(r+8));while(n<192)if(r[8+n++])return -1;return 1;
}
inline uint32_t identity(const unsigned char* r,unsigned seq){return !std::memcmp(r,"ADD1",4)?0x80000000u|seq:number(r+4);}
inline bool deleted(const unsigned char* r){return !std::memcmp(r,"DEL2",4);}
inline uint32_t align(uint32_t v,uint32_t a){return (v+a-1)/a*a;}
inline uint32_t dir_sectors(uint32_t blocks){return (blocks+DIR_ENTRIES-1)/DIR_ENTRIES;}
}
// Version 4 (GVDIDX04): each direction is a run of sorted 4 KiB blocks found
// through a CRC-checked sector directory holding each block's first folded key.
class Dictionary {
public:
 struct Range {uint32_t begin=0,end=0,comparisons=0;};
 Dictionary()=default;
 Dictionary(DictionarySource* source,int index):source_(source),index_(index){
  if(!source_||!source_->read(index_,0,h_,160)||std::memcmp(h_,"GVDIDX04",8)||dict_format::crc(h_,156)!=num(156))return;
  using namespace dict_format;
  auto n=num(12),end=num(8),b0=num(136),b1=num(148);
  if(!n||n>MAX_ENTRIES||end>BASE_LIMIT||end>source_->size(index_)||num(120)!=BLOCK||num(124)!=SECTOR)return;
  if(!b0||!b1||b0>n||b1>n||b0>BASE_LIMIT/BLOCK||b1>BASE_LIMIT/BLOCK)return;
  uint32_t d0=align(SECTOR+4*n,SECTOR),d1=d0+dir_sectors(b0)*SECTOR,s0=align(d1+dir_sectors(b1)*SECTOR,BLOCK),s1=s0+b0*BLOCK;
  if(num(128)!=d0||num(132)!=s0||num(140)!=d1||num(144)!=s1||end!=s1+b1*BLOCK)return;
  const unsigned starts[]={16,48,60,72,96},lengths[]={32,12,12,24,24};
  for(unsigned i=0;i<5;++i){const char* p=reinterpret_cast<const char*>(h_+starts[i]);unsigned j=0;while(j<lengths[i]&&p[j]){if(static_cast<unsigned char>(p[j])<32||p[j]==127)return;++j;}if(!j||j==lengths[i]||!writer::valid_utf8(p,j))return;while(j<lengths[i])if(p[j++])return;}
  for(int side=0;side<2;++side){const char* p=code(side);if(*p<'a'||*p>'z')return;for(;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='-'))return;}
  if(!std::strcmp(code(0),code(1)))return;
  dir_[0]=d0;dir_[1]=d1;blocks_[0]=s0;blocks_[1]=s1;nblocks_[0]=b0;nblocks_[1]=b1;
  valid_=true;
 }
 bool valid() const{return valid_;}
 // Older indexed formats are recognized only to ask for a PC Save As upgrade.
 bool legacy_index() const{return (!std::memcmp(h_,"GVDIDX01",8)||!std::memcmp(h_,"GVDIDX02",8)||!std::memcmp(h_,"GVDIDX03",8))&&dict_format::crc(h_,156)==num(156);}
 bool editable() const{return valid_;}
 // Locate an immutable canonical row in either sorted direction. The
 // canonical ordinal breaks duplicate-key ties.
 uint32_t rank(int side,uint32_t row) const {return rank(side,row,nullptr);}
private:
 friend class DictionaryAdditions;
 uint32_t rank(int side,uint32_t row,const char* validated_key) const {
  // Only the immediate suppression caller supplies an already fully checked key.
  char a[192],b[192];uint32_t front_rank=count();
  if(!validated_key||!side){if(!read(row,a,b,&front_rank))return count();if(!side)return front_rank;validated_key=b;}
  Query q;q.mode=Query::before_row;q.text=validated_key;q.row=row;fold(q);
  uint32_t comparisons=0,r=partition(1,q,0,count(),comparisons);
  if(r>=count()||row_at(1,r)!=row){fail();return count();}
  return r;
 }
 public:
 const char* name() const{return reinterpret_cast<const char*>(h_+16);}
 const char* code(int side) const{return reinterpret_cast<const char*>(h_+(side?60:48));}
 const char* label(int side) const{return reinterpret_cast<const char*>(h_+(side?96:72));}
 uint32_t count() const{return valid_?num(12):0;}
 uint32_t base_end() const{return num(8);}
 uint32_t file_size() const{return source_?source_->size(index_):0;}
 bool bytes(uint32_t at,void* out,unsigned n) const{return valid_&&source_->read(index_,at,out,n);}
 int index() const{return index_;}
 DictionarySource* source() const{return source_;}
 uint32_t row_at(int side,uint32_t rank) const {Entry e;if(!entry_at(side,rank,e)){failed_=true;return count();}return e.row;}
 // Read the entry at a sorted position (the display path: one cached block).
 bool read_rank(int side,uint32_t rank,char front[192],char back[192],uint32_t* row=nullptr) const {
  Entry e;if(!entry_at(side,rank,e))return fail();
  std::memcpy(front,e.front,e.front_n);front[e.front_n]=0;std::memcpy(back,e.back,e.back_n);back[e.back_n]=0;
  if(row)*row=e.row;
  return true;
 }
 // Read a canonical row: the record table gives its front-order rank, and the
 // entry found there must name the same row.
 bool read(uint32_t row,char front[192],char back[192],uint32_t* front_rank=nullptr) const {
  unsigned char cell[4];
  if(row>=count()||!bytes(dict_format::SECTOR+row*4,cell,4))return fail();
  uint32_t rank=dict_format::number(cell),found=count();
  if(rank>=count()||!read_rank(0,rank,front,back,&found)||found!=row)return fail();
  if(front_rank)*front_rank=rank;
  return true;
 }
 static unsigned fold(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
 static int compare_prefix(const char* word,const char* query){return dictionary_fold::compare(word,query,true);}
 // Prefix bounds. A hint (an earlier range for a shorter prefix of this query)
 // narrows both searches; an empty query needs no reads at all.
 Range prefix(int side,const char* query,const Range* within=nullptr) const {
  Range r;failed_=false;if(!valid_)return r;
  uint32_t lo=0,hi=count();
  if(!*query){r.end=hi;return r;}
  if(within&&within->begin<=within->end&&within->end<=hi){lo=within->begin;hi=within->end;}
  Query q;q.text=query;fold(q);
  q.mode=Query::before_prefix;uint32_t begin=partition(side,q,lo,hi,r.comparisons);if(failed_)return {};
  q.mode=Query::not_after_prefix;uint32_t end=partition(side,q,begin,hi,r.comparisons);if(failed_)return {};
  r.begin=begin;r.end=end;return r;
 }
 bool failed() const{return failed_;}
private:
 struct Query {enum Mode{before_prefix,not_after_prefix,before_row} mode=before_prefix;const char* text="";unsigned char folded[256];unsigned folded_n=0;uint32_t row=0;};
 struct Directory {uint32_t first=0;unsigned count=0,key_n=0;bool truncated=false;unsigned char key[dict_format::DIR_KEY];};
 struct Entry {uint32_t row=0;const char* front=nullptr;const char* back=nullptr;unsigned front_n=0,back_n=0;};
 static void fold(Query& q){
  dictionary_fold::Iterator it(q.text);unsigned n=0;
  for(unsigned c;(c=it.next())!=0;){
   unsigned char u[4];unsigned k=0;
   if(c>=0x110000)u[k++]=static_cast<unsigned char>(c-0x110000);
   else if(c<0x80)u[k++]=c;
   else if(c<0x800){u[k++]=0xc0|(c>>6);u[k++]=0x80|(c&63);}
   else if(c<0x10000){u[k++]=0xe0|(c>>12);u[k++]=0x80|((c>>6)&63);u[k++]=0x80|(c&63);}
   else {u[k++]=0xf0|(c>>18);u[k++]=0x80|((c>>12)&63);u[k++]=0x80|((c>>6)&63);u[k++]=0x80|(c&63);}
   if(n+k>sizeof q.folded)break;
   std::memcpy(q.folded+n,u,k);n+=k;
  }
  q.folded_n=n;
 }
 static bool before(const Query& q,const char* text,uint32_t row){
  int c=dictionary_fold::compare(text,q.text,q.mode!=Query::before_row);
  if(q.mode==Query::before_prefix)return c<0;
  if(q.mode==Query::not_after_prefix)return c<=0;
  return c<0||(!c&&row<q.row);
 }
 // Decide the predicate for a block's first key from the directory alone:
 // 1 before, 0 not before, -1 needs the block (truncated key or equal tie).
 static int before(const Query& q,const Directory& d){
  unsigned common=d.key_n<q.folded_n?d.key_n:q.folded_n;int c=std::memcmp(d.key,q.folded,common);
  if(c)return c<0;
  if(q.folded_n<=d.key_n){
   if(q.mode==Query::before_prefix)return 0;
   if(q.mode==Query::not_after_prefix)return 1;
   return q.folded_n<d.key_n||d.truncated?0:-1;
  }
  return d.truncated?-1:1;
 }
 const unsigned char* sector(uint32_t at) const {
  auto& w=source_->workspace();++w.clock;
  if(w.reuse)for(unsigned i=0;i<w.SECTORS;++i)if(w.sector_owner[i]==source_&&w.sector_file[i]==index_&&w.sector_at[i]==at){w.sector_used[i]=w.clock;return w.sector+i*dict_format::SECTOR;}
  unsigned slot=0;for(unsigned i=0;i<w.SECTORS;++i){if(!w.sector_owner[i]){slot=i;break;}if(w.sector_used[i]<w.sector_used[slot])slot=i;}
  unsigned char* p=w.sector+slot*dict_format::SECTOR;w.sector_owner[slot]=nullptr;++w.misses;
  if(!bytes(at,p,dict_format::SECTOR)||dict_format::crc(p+4,dict_format::SECTOR-4)!=dict_format::number(p))return nullptr;
  w.sector_owner[slot]=source_;w.sector_file[slot]=index_;w.sector_at[slot]=at;w.sector_used[slot]=w.clock;return p;
 }
 bool directory(int side,uint32_t b,Directory& d) const {
  using namespace dict_format;
  if(b>=nblocks_[side])return fail();
  const unsigned char* p=sector(dir_[side]+(b/DIR_ENTRIES)*SECTOR);if(!p)return fail();
  p+=4+(b%DIR_ENTRIES)*32;
  d.first=number(p);d.count=half(p+4);d.key_n=p[6];d.truncated=p[7]==1;std::memcpy(d.key,p+8,DIR_KEY);
  if(!d.count||d.first>=count()||d.count>count()-d.first||(!b&&d.first)||!d.key_n||d.key_n>DIR_KEY||p[7]>1||(d.truncated&&d.key_n!=DIR_KEY))return fail();
  return true;
 }
 const unsigned char* block(int side,uint32_t b,const Directory& d) const {
  using namespace dict_format;
  auto& w=source_->workspace();++w.clock;uint32_t at=blocks_[side]+b*BLOCK;
  if(w.reuse)for(unsigned i=0;i<w.BLOCKS;++i)if(w.block_owner[i]==source_&&w.block_file[i]==index_&&w.block_at[i]==at){w.block_used[i]=w.clock;return w.block[i];}
  unsigned slot=!w.block_owner[0]?0:!w.block_owner[1]?1:w.block_used[0]<=w.block_used[1]?0:1;
  unsigned char* p=w.block[slot];w.block_owner[slot]=nullptr;++w.misses;
  if(!bytes(at,p,BLOCK)||crc(p+4,BLOCK-4)!=number(p)||half(p+4)!=d.count||p[6]||p[7]){fail();return nullptr;}
  unsigned previous=8+2*d.count;if(previous>=BLOCK||half(p+8)!=previous){fail();return nullptr;}
  for(unsigned i=1;i<d.count;++i){unsigned o=half(p+8+2*i);if(o<=previous+5||o>=BLOCK){fail();return nullptr;}previous=o;}
  w.block_owner[slot]=source_;w.block_file[slot]=index_;w.block_at[slot]=at;w.block_used[slot]=w.clock;return p;
 }
 // Validate one entry in place: row, two NUL-terminated fields, row grammar.
 bool entry(const unsigned char* p,const Directory& d,unsigned i,Entry& e) const {
  unsigned start=dict_format::half(p+8+2*i),limit=i+1<d.count?dict_format::half(p+10+2*i):dict_format::BLOCK;
  if(start+5>limit)return fail();
  e.row=unsigned(p[start])|(unsigned(p[start+1])<<8)|(unsigned(p[start+2])<<16);
  const char* s=reinterpret_cast<const char*>(p);unsigned a=start+3,z=a;while(z<limit&&s[z])++z;if(z>=limit)return fail();
  unsigned b=z+1,y=b;while(y<limit&&s[y])++y;if(y>=limit||(i+1<d.count&&y+1!=limit))return fail();
  e.front=s+a;e.front_n=z-a;e.back=s+b;e.back_n=y-b;
  if(e.row>=count()||e.front_n+1+e.back_n>191||!dict_format::field_ok(e.front,e.front_n)||!dict_format::field_ok(e.back,e.back_n))return fail();
  return true;
 }
 bool block_of(int side,uint32_t rank,uint32_t& b,Directory& d) const {
  uint32_t lo=0,hi=nblocks_[side];
  while(hi-lo>1){uint32_t mid=lo+(hi-lo)/2;if(!directory(side,mid,d))return false;if(d.first<=rank)lo=mid;else hi=mid;}
  if(!directory(side,lo,d)||rank<d.first||rank-d.first>=d.count)return fail();
  b=lo;return true;
 }
 bool entry_at(int side,uint32_t rank,Entry& e) const {
  uint32_t b=0;Directory d;if(!valid_||rank>=count()||!block_of(side,rank,b,d))return fail();
  const unsigned char* p=block(side,b,d);return p&&entry(p,d,rank-d.first,e);
 }
 // First rank in [lo,hi) whose entry is not "before" the query, else hi.
 uint32_t partition(int side,const Query& q,uint32_t lo,uint32_t hi,uint32_t& comparisons) const {
  if(lo>=hi)return lo;
  Directory d;Entry e;uint32_t first=0,last=nblocks_[side]-1;
  if(lo&&!block_of(side,lo,first,d))return hi;
  if(hi<count()&&!block_of(side,hi-1,last,d))return hi;
  uint32_t a=first+1,z=last+1;
  while(a<z){
   uint32_t m=a+(z-a)/2;++comparisons;if(!directory(side,m,d))return hi;
   int decided=before(q,d);
   if(decided<0){const unsigned char* p=block(side,m,d);if(!p||!entry(p,d,0,e))return hi;decided=before(q,side?e.back:e.front,e.row);}
   if(decided)a=m+1;else z=m;
  }
  uint32_t c=a-1;if(!directory(side,c,d))return hi;
  const unsigned char* p=block(side,c,d);if(!p)return hi;
  uint32_t i0=lo>d.first?lo-d.first:0,i1=(hi-d.first<d.count?hi-d.first:d.count);
  if(i0>i1){fail();return hi;}
  while(i0<i1){uint32_t m=i0+(i1-i0)/2;++comparisons;if(!entry(p,d,m,e))return hi;if(before(q,side?e.back:e.front,e.row))i0=m+1;else i1=m;}
  return d.first+i0;
 }
 bool fail() const{failed_=true;return false;}
 uint32_t num(unsigned at) const{return dict_format::number(h_+at);}
 DictionarySource* source_=nullptr;int index_=0;unsigned char h_[160]={};bool valid_=false;mutable bool failed_=false;
 uint32_t dir_[2]={},blocks_[2]={},nblocks_[2]={};
};
#if defined(__DEVKITARM__) || defined(VOCAB_HOST_FATFS)
#include "ff.h"
class DictionaryCatalog final:public DictionarySource {
public:
 static constexpr int CAPACITY=24;
 explicit DictionaryCatalog(bool available,FIL& file,bool& opened);
 ~DictionaryCatalog();
 DictionaryCatalog(const DictionaryCatalog&)=delete;
 DictionaryCatalog& operator=(const DictionaryCatalog&)=delete;
 int count() const{return count_;}
 const char* filename(int i)const{return i>=0&&i<count_?names_[i]:"";}
 Dictionary dictionary(int i){return i>=0&&i<count_?Dictionary(this,i):Dictionary();}
 bool has_legacy(){for(int i=0;i<count_;++i)if(dictionary(i).legacy_index())return true;return false;}
 int match(int i,const char* a,const char* b){auto d=dictionary(i);if(!d.valid())return -1;if(!std::strcmp(a,d.code(0))&&!std::strcmp(b,d.code(1)))return 0;if(!std::strcmp(a,d.code(1))&&!std::strcmp(b,d.code(0)))return 1;return -1;}
 bool read(int,uint32_t,void*,unsigned) override;
 uint32_t size(int) override;
 DictionaryWorkspace& workspace() override;
 bool append(int,const unsigned char*,unsigned seq);
 bool refresh(int);
 const char* error() const{return error_;}
private:
 bool activate(int,bool write=false);bool close();void invalidate(){cache_at_=~0u;cache_n_=0;}
 char names_[CAPACITY][64]={};FIL& file_;bool& opened_;bool writable_=false;int active_=-1,count_=0;
 unsigned char cache_[256]={};uint32_t cache_at_=~0u;unsigned cache_n_=0;const char* error_="";
};
#endif
