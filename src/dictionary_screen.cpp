#include "dictionary_screen.h"
#include "dictionary.h"
#include "dictionary_handle.h"
#include "dictionary_search.h"
#include "dictionary_choice.h"
#include "entry_editor.h"
#include "entry_screen.h"
#include "entry_render.h"
#include "render.h"
#include "entries_screen.h"
#include "import_screen.h"
#include "dict_build.h"
#include "bn_core.h"
#include "bn_keypad.h"
namespace {
BN_DATA_EWRAM_BSS EntryEditor query(entry_glyph_width);
BN_DATA_EWRAM_BSS DictionaryAdditions additions;
uint16_t keys() {
 const bool held[]={bn::keypad::up_held(),bn::keypad::down_held(),bn::keypad::left_held(),bn::keypad::right_held(),bn::keypad::a_held(),bn::keypad::b_held(),bn::keypad::l_held(),bn::keypad::r_held(),bn::keypad::start_held(),bn::keypad::select_held()};
 uint16_t v=0;for(int i=0;i<10;++i)if(held[i])v|=1u<<i;return v;
}
void release(){while(keys())bn::core::update();}
void notice(Canvas& p,const char* a,const char* b){
 const char* lines[]={a,b};
 release();run_notice(p,"Dictionary",lines,2);release();
}
void number(char* out,uint32_t n){char digits[11];int i=0;do{digits[i++]=char('0'+n%10);n/=10;}while(n);int j=0;while(i)out[j++]=digits[--i];out[j]=0;}
__attribute__((noinline)) bool mutate_dictionary(Canvas& p,Dictionary d,EntryMutation operation,uint32_t identity,const char* front,const char* back){
 if(!d.editable()){notice(p,"Upgrade .dict with PC Save As","Use PC builder 2.0 or newer.");return false;}
 auto& add_editor=entry_draft_editor();
 EntryEditorLoan loan(add_editor);
 if(!add_editor.open_dictionary_mutation(operation,front,back))return false;
 while(add_editor.active()){
  add_editor.frame(keys());
  if(add_editor.screen()==EntryEditor::Screen::menu){release();return false;}
  if(add_editor.commit_requested()){
   entries_show_saving(p);
   bool saved=operation==EntryMutation::add?additions.append(add_editor.row()):
     operation==EntryMutation::edit?additions.replace(identity,add_editor.row()):additions.remove(identity);
   add_editor.finish(saved,additions.error());
   if(saved){notice(p,"Entry saved to dictionary.","Stored in its .dict file.");return true;}
  }
  p.clear();
  render_entry(add_editor,p.pixels,[](void* ctx,int x,int y,const char* text){
   auto& add_editor=entry_draft_editor();
   auto& canvas=*static_cast<Canvas*>(ctx);
   if(y==0&&add_editor.screen()!=EntryEditor::Screen::confirm_delete)text=add_editor.screen()==EntryEditor::Screen::front?"Dictionary entry 1/2":"Dictionary entry 2/2";
   if(y==18)return;
   if(y==144&&add_editor.screen()==EntryEditor::Screen::back)text="Start+A: Save  Start+B: Back";
   if(x<0)canvas.ui_center(y,text);else canvas.ui(x,y,text);
  },&p);
  p.body(8,18,d.label(add_editor.screen()==EntryEditor::Screen::back?1:0));p.flip();
 }
 return false;
}

// Dictionary list entries: the whole dictionary in alphabetical order of the
// shown language, live additions/edits merged in at their sorted places.
// order[k]: the k-th addition in sorted order; at[k]: its row in the list.
class DictionaryList final : public EntriesSource {
public:
 DictionaryList(DictionarySearch& search,int side):search_(search),side_(side){
  auto* scratch=entries_order_scratch();
  order_=reinterpret_cast<uint16_t*>(scratch);at_=reinterpret_cast<uint32_t*>(scratch+2*DictionaryAdditions::LIMIT);
  build();
 }
 __attribute__((noinline)) void build(){
  search_.search(side_,"");added_=search_.added();
  const Dictionary& d=search_.dictionary();
  // Insertion point of each addition among the visible base rows.
  for(unsigned k=0;k<added_;++k){
   char f[192],b[192];order_[k]=uint16_t(k);at_[k]=0;
   if(!search_.read(k,f,b))continue;
   auto range=d.prefix(side_,side_?b:f);
   at_[k]=search_.visible_before(range.begin);
  }
  // Stable insertion sort by insertion point, then by the folded text.
  for(unsigned i=1;i<added_;++i)for(unsigned j=i;j>0&&before(j,j-1);--j){
   uint16_t o=order_[j];order_[j]=order_[j-1];order_[j-1]=o;uint32_t a=at_[j];at_[j]=at_[j-1];at_[j-1]=a;
  }
  for(unsigned k=0;k<added_;++k)at_[k]+=k;   // visible base index -> list row
 }
 uint32_t count() override{return search_.count();}
 // List row -> search position (additions first, then visible base rows).
 uint32_t position(uint32_t row) const{
  unsigned lo=0,hi=added_;while(lo<hi){unsigned m=(lo+hi)/2;if(at_[m]<row)lo=m+1;else hi=m;}
  if(lo<added_&&at_[lo]==row)return order_[lo];
  return added_+(row-lo);
 }
 uint32_t row_of(uint32_t position) const{
  if(position<added_){for(unsigned k=0;k<added_;++k)if(order_[k]==position)return at_[k];return 0;}
  uint32_t v=position-added_;unsigned n=0;while(n<added_&&at_[n]-n<=v)++n;return v+n;
 }
 bool read(uint32_t row,char f[192],char b[192]){return search_.read(position(row),f,b);}
 uint32_t identity(uint32_t row){return search_.identity(position(row));}
 bool text(uint32_t row,char out[192]) override{
  char f[192],b[192];if(!read(row,f,b))return false;
  const char* first=side_?b:f;const char* second=side_?f:b;
  unsigned x=std::strlen(first),y=std::strlen(second);if(x>188)x=188;if(x+3+y>191)y=191-3-x;
  while(x&&(first[x]&0xc0)==0x80){--x;}   // whole UTF-8 characters
  while(y&&(second[y]&0xc0)==0x80){--y;}
  std::memcpy(out,first,x);std::memcpy(out+x," = ",3);std::memcpy(out+x+3,second,y);out[x+3+y]=0;return true;
 }
 bool alphabetical() override{return true;}
 // Letter group: the first folded character; digits and symbols are "#".
 bool letter(uint32_t row,uint32_t& key,char label[8]) override{
  char f[192],b[192];if(!read(row,f,b))return false;
  return entries_letter_of(side_?b:f,key,label);
 }
 // Start: the other language first, the same entry stays selected.
 bool swap(uint32_t& sel) override{
  uint32_t position=this->position(sel),row=~0u;unsigned addition=~0u;
  if(position<added_)addition=position;else row=search_.identity(position);
  side_^=1;build();
  if(addition!=~0u){sel=row_of(addition);return true;}
  const Dictionary& d=search_.dictionary();
  uint32_t rank=d.rank(side_,row);
  sel=rank<d.count()?row_of(added_+search_.visible_before(rank)):0;
  return true;
 }
 int side() const{return side_;}
private:
 bool before(unsigned a,unsigned b){
  if(at_[a]!=at_[b])return at_[a]<at_[b];
  char fa[192],ba[192],fb[192],bb[192];
  if(!search_.read(order_[a],fa,ba)||!search_.read(order_[b],fb,bb))return false;
  return dictionary_fold::compare(side_?ba:fa,side_?bb:fb)<0;
 }
 DictionarySearch& search_;int side_;unsigned added_=0;uint16_t* order_=nullptr;uint32_t* at_=nullptr;
};
static_assert(DictionaryAdditions::LIMIT*6<=4096,"order scratch");

__attribute__((noinline)) void delete_dictionary_entry(Canvas& p,uint32_t identity){
 entries_show_saving(p);
 if(!additions.remove(identity))notice(p,"Delete failed.",additions.error());
}

// Returns with the search at its previous side and an empty query result.
__attribute__((noinline)) void dictionary_list_entries(Canvas& p,Dictionary d,DictionarySearch& search,int side){
 DictionaryList list(search,side);
 entries::Nav nav(list.count());
 static const char* const items[]={"Edit entry","Delete entry"};
 static const char* const confirm[]={nullptr,"Delete entry? A: Yes  B: No"};
 while(run_entries(p,"Dictionary list entries",list,nav,entries_row_cache())==EntriesResult::open){
  char front[192],back[192];uint32_t row=nav.sel();
  if(!list.read(row,front,back))continue;
  uint32_t identity=list.identity(row);
  int choice=run_menu(p,"Dictionary entry",items,confirm,2);
  if(choice==0)mutate_dictionary(p,d,EntryMutation::edit,identity,front,back);
  else if(choice==1)delete_dictionary_entry(p,identity);
  else {release();continue;}
  list.build();nav.resize(list.count());release();
 }
 release();
}
}

namespace {
// The dictionary opened last (main menu > Dictionaries marks it with ▶).
BN_DATA_EWRAM_BSS char last_dictionary[64];
void remember_opened(const char* name){std::strncpy(last_dictionary,name,63);last_dictionary[63]=0;}
// Dictionaries as a gbamp3-style list: "Name  (de / en)", ▶ on mark.
// subset: catalog indices to show (A to Z, as the catalog); null: all valid.
class DictionaryChoices final : public EntriesSource {
public:
 DictionaryChoices(DictionaryCatalog& catalog,const int* subset=nullptr,int n=0):catalog_(catalog){
  if(subset)for(int k=0;k<n;++k)index_[count_++]=subset[k];
  else for(int i=0;i<catalog.count();++i)if(catalog.dictionary(i).valid())index_[count_++]=i;
 }
 uint32_t count() override{return uint32_t(count_+extra_count_);}
 // V4.4.3: rows after the dictionaries ("Type codes", "Later").
 void extras(const char* const* rows,int n){extra_=rows;extra_count_=n;}
 bool text(uint32_t row,char out[192]) override{
  if(row>=uint32_t(count_)){if(row<uint32_t(count_+extra_count_)){std::strcpy(out,extra_[row-count_]);return true;}return false;}
  auto d=catalog_.dictionary(index_[row]);
  std::strcpy(out,d.name());std::strcat(out,"  (");std::strcat(out,d.code(0));std::strcat(out," / ");std::strcat(out,d.code(1));std::strcat(out,")");
  return true;
 }
 int icon(uint32_t row) override{return mark_[0]&&row<uint32_t(count_)&&!std::strcmp(catalog_.filename(index_[row]),mark_)?ICON_PLAY:ICON_NONE;}
 void mark(const char* name){std::strncpy(mark_,name?name:"",63);mark_[63]=0;}
 int index(int row) const{return index_[row];}
 int find(const char* name) const{for(int k=0;k<count_;++k)if(name&&name[0]&&!std::strcmp(catalog_.filename(index_[k]),name))return k;return -1;}
 int size() const{return count_;}
private:
 DictionaryCatalog& catalog_;int index_[DictionaryCatalog::CAPACITY]={};int count_=0;char mark_[64]={};
 const char* const* extra_=nullptr;int extra_count_=0;
};

// V4.4.3 list languages. A list's two codes say which language each TXT
// column holds; they live in GBAVOCAB.SYS, the TXT is never changed.
bool pair_fits(const PairMetadata& list,const char* a,const char* b){
 return list.present()&&((!std::strcmp(list.front,a)&&!std::strcmp(list.back,b))||(!std::strcmp(list.front,b)&&!std::strcmp(list.back,a)));
}
void set_list_pair(VocabFile& vf,const char* a,const char* b,int column){
 if(column==0)vf.languages.set(a,b);else vf.languages.set(b,a);
 vf.pair_dirty=true;
}
uint32_t s_sample_seed=0x2545F491u;
// Which column holds the language `label`? An empty list asks nothing (the
// front column). Otherwise one random card asks "Which is German?" with both
// words as rows and "Other card" for another one; B cancels (-1).
int ask_column(Canvas& p,const VocabFile& vf,const char* title,const char* label){
 if(vf.line_count<=0)return 0;
 int last=-1,unreadable=0;
 for(;;){
  s_sample_seed=s_sample_seed*1664525u+1013904223u;
  int index=int((s_sample_seed>>8)%uint32_t(vf.line_count));
  if(index==last&&vf.line_count>1)index=(index+1)%vf.line_count;
  last=index;
  char raw[VOCAB_RAW_LINE_MAX];
  char* tab=vocab_file_raw_row(vf,nullptr,0,index,raw)?std::strchr(raw,'\t'):nullptr;
  if(!tab){if(++unreadable>=8)return 0;continue;}   // no readable card: front column
  *tab=0;char* back=tab+1;if(char* extra=std::strchr(back,'\t'))*extra=0;
  char pair[VOCAB_RAW_LINE_MAX+4];std::strcpy(pair,raw);std::strcat(pair," = ");std::strcat(pair,back);
  char question[40]="Which is ";append_limited(question,label,23);std::strcat(question,"?");   // labels: 1..23 bytes
  const char* lines[]={pair,question};
  const char* items[]={raw,back,"Other card"};
  int answer=run_choice(p,title,lines,2,items,3);
  release();
  if(answer!=2)return answer;
 }
}
// The list takes a dictionary's languages; asks which column is which
// unless the codes already match (either order). chosen: picked by hand in
// the Entry editor, so a list with cards is always asked (this also fixes a
// list whose codes are the wrong way round). False: cancelled.
bool dictionary_languages(Canvas& p,VocabFile& vf,Dictionary d,const char* title,bool chosen=false){
 if(pair_fits(vf.languages,d.code(0),d.code(1))&&!(chosen&&vf.line_count>0))return true;
 // "Which is German?": the dictionary's label, or the name for a known code
 // when the label is just the code.
 char label[24];std::strcpy(label,d.label(0));
 if(!std::strcmp(label,d.code(0)))dict_build_label(d.code(0),label);
 int column=ask_column(p,vf,title,label);
 if(column<0)return false;
 set_list_pair(vf,d.code(0),d.code(1),column);return true;
}
// Type codes: two codes typed in any order, then (with cards) which is which,
// also when they are the list's codes already (typed by hand: may fix them).
bool type_list_codes(Canvas& p,VocabFile& vf){
 char a[12],b[12];
 if(!ask_language_codes(p,a,b,true))return false;
 if(pair_fits(vf.languages,a,b)&&vf.line_count<=0)return true;
 char label[24];dict_build_label(a,label);
 int column=ask_column(p,vf,"List languages",label);
 if(column<0)return false;
 set_list_pair(vf,a,b,column);return true;
}
// Dictionaries list over eligible[0..count); returns the chosen k or -1 (B).
__attribute__((noinline)) int choose_dictionary(Canvas& p,DictionaryCatalog& catalog,const int* eligible,int count,int current,const char* mark){
 DictionaryChoices choices(catalog,eligible,count);choices.mark(mark);
 entries::Nav nav(choices.count());
 int marked=choices.find(mark);nav.select(uint32_t(marked>=0?marked:current));
 int k=run_entries(p,"Dictionaries",choices,nav,entries_row_cache())==EntriesResult::open?int(nav.sel()):-1;
 release();return k;
}
}

static __attribute__((noinline)) bool dictionary_screen_inner(Renderer& renderer,VocabFile* target,DictionaryResult& result,DictionarySearch& search,const char* preferred) {
 additions.set_available(vocab_file_sd_ready());
 // Three results of two lines each, in the borrowed list row cache.
 auto visible_rows=reinterpret_cast<char(*)[2][192]>(entries_row_cache());
 const PairMetadata* filter=target?&target->languages:nullptr;
 renderer.reset();bn::core::update();bn::core::update();
 DictionaryCatalog catalog(vocab_file_sd_ready(),vocab_file_dictionary_fil(),vocab_file_dictionary_opened());
 DictionaryChoice selection(catalog,target);auto& eligible=selection.eligible;auto& count=selection.count;
 // Main menu > Dictionaries: the chosen file opens directly.
 if(preferred)for(int k=0;k<count;++k)if(!std::strcmp(catalog.filename(eligible[k]),preferred)){selection.choice=k;selection.chooser=false;}
 bool picked=false;
 {
  Canvas p;
  if(target&&target->pair_blocked)notice(p,"List metadata needs repair.","Back up TXT/SAV; repair on PC.");
  else if(!count&&catalog.has_legacy())notice(p,"Upgrade .dict with PC Save As","Use PC builder 2.0 or newer.");
  else if(!count)notice(p,catalog.count()?"No dictionary for this list pair.":"No valid .dict in /gbavocab.",catalog.count()?"Use the PC builder to add it.":"Copy .dict from the PC builder.");
  else {
   auto& choice=selection.choice;int side=0;bool chooser=selection.chooser,done=false,wait=true;
   if(target&&!chooser)selection.remember(catalog,*target);
   Dictionary d=catalog.dictionary(eligible[choice]);
   if(filter&&filter->present())side=catalog.match(eligible[choice],filter->front,filter->back);
   search.open(d);search.search(side,"");uint32_t selected=0;
   query.open_lookup(!target);char previous[192]={};bool refresh=true;uint32_t cached_top=~0u;
   // Typing waits a few frames before searching so keys and the caret are
   // drawn at once; any result action searches the current text first.
   constexpr unsigned SEARCH_SETTLE_FRAMES=4;unsigned pending=0;
   // Start+R: the new dictionary's name replaces the status line for 2 s.
   constexpr unsigned NAME_FRAMES=120;unsigned name_frames=0;
   if(!chooser)remember_opened(catalog.filename(eligible[choice]));
   while(!done) {
    if(chooser) {
     // The list marks the linked dictionary, else the one opened last.
     const char* mark=target&&target->settings.dictionary[0]?target->settings.dictionary:last_dictionary;
     int k=choose_dictionary(p,catalog,eligible,count,choice,mark);
     refresh=true;cached_top=~0u;wait=true;chooser=false;   // the list used the row cache
     if(k<0){done=true;continue;}
     choice=k;d=catalog.dictionary(eligible[choice]);
     search.open(d);remember_opened(catalog.filename(eligible[choice]));
     if(target)selection.remember(catalog,*target);
     side=filter&&filter->present()?catalog.match(eligible[choice],filter->front,filter->back):0;
     search.search(side,query.text().data());selected=0;
     continue;
    }
    auto held=keys();
    if(wait){if(!held){wait=false;query.frame(0);}}
    else {
     query.frame(held);
     using A=EntryEditor::LookupAction;
     auto action=query.take_lookup_action();
     if(pending&&action!=A::none){std::strcpy(previous,query.text().data());search.search(side,previous);selected=0;refresh=true;pending=0;}
     switch(action) {
     case A::cancel:done=true;break;
     case A::up:if(selected>0)--selected;break;
     case A::down:if(selected+1<search.count())++selected;break;
     case A::direction:side^=1;search.search(side,query.text().data());selected=0;refresh=true;break;
     case A::chooser:
      // Start+R: the next allowed dictionary (A to Z, wrapping), same text.
      // Only what is shown changes; the list's link stays.
      if(count>1){
       choice=(choice+1)%count;d=catalog.dictionary(eligible[choice]);
       search.open(d);remember_opened(catalog.filename(eligible[choice]));
       side=filter&&filter->present()?catalog.match(eligible[choice],filter->front,filter->back):0;
       search.search(side,query.text().data());selected=0;refresh=true;pending=0;name_frames=NAME_FRAMES;
      }
      break;
     case A::add:{
      // Start+Select: the dictionary entry menu.
      static const char* const items[]={"Add entry","Edit entry","Delete entry","Show Dictionary List entries"};
      static const char* const confirm[]={nullptr,nullptr,"Delete entry? A: Yes  B: No",nullptr};
      int item=run_menu(p,"Dictionary",items,confirm,4);
      bool has=search.count()&&search.read(selected,result.front,result.back);
      uint32_t identity=has?search.identity(selected):~0u;
      if(item==0)mutate_dictionary(p,d,EntryMutation::add,~0u,"","");
      else if((item==1||item==2)&&!has)notice(p,"No entry selected.","Type to find an entry first.");
      else if(item==1)mutate_dictionary(p,d,EntryMutation::edit,identity,result.front,result.back);
      else if(item==2)delete_dictionary_entry(p,identity);
      else if(item==3)dictionary_list_entries(p,d,search,side);
      search.search(side,query.text().data());if(item>=0)selected=0;refresh=true;wait=true;
      break;}
     case A::select:
      if(search.read(selected,result.front,result.back)) {
       result.languages.set(d.code(0),d.code(1));std::strcpy(result.dictionary,catalog.filename(eligible[choice]));picked=true;done=true;
      }break;
     default:break;
     }
     if(std::strcmp(previous,query.text().data())) {std::strcpy(previous,query.text().data());pending=SEARCH_SETTLE_FRAMES;}
     else if(pending&&!--pending){search.search(side,previous);selected=0;refresh=true;}
    }
    p.clear();
    {
     // No header and no instructions: three results fit (V4.0).
     constexpr int RESULTS=3,RESULT_Y=22,RESULT_H=38,STATUS_Y=140;
     auto caret=query.layout().position(query.text(),query.text().caret_byte());
     auto start=query.layout().row_content_start(query.text(),caret.row);
     p.body(8,1,query.text().data()+start,220);if(query.caret_visible())p.caret(8+caret.x,1);
     p.fill(0,19,240,1,ui_colors::LIGHT);
     if(!search.count())p.body(8,40,"No prefix matches");
     uint32_t top=(selected/RESULTS)*RESULTS;
     if(refresh||cached_top!=top){
      std::memset(visible_rows,0,RESULTS*2*192);
      for(unsigned i=0;i<unsigned(RESULTS)&&top+i<search.count();++i)search.read(top+i,visible_rows[i][0],visible_rows[i][1]);
      refresh=false;cached_top=top;
     }
     for(uint32_t i=top;i<top+RESULTS&&i<search.count();++i){int y=RESULT_Y+int(i-top)*RESULT_H;
      if(i==selected)p.fill(0,y,240,RESULT_H-3,ui_colors::BLUE);
      p.body(8,y+1,visible_rows[i-top][side],224);p.body(8,y+18,visible_rows[i-top][side^1],224);
     }
     p.fill(0,STATUS_Y-3,240,1,ui_colors::LIGHT);
     if(additions.error()[0]||search.failed())p.body(8,STATUS_Y,"Dictionary I/O or format error");
     else if(name_frames){--name_frames;p.body(8,STATUS_Y,d.name(),224);}
     else {
      char status[64];std::strcpy(status,d.code(side));std::strcpy(status+std::strlen(status)," > ");std::strcpy(status+std::strlen(status),d.code(side^1));std::strcpy(status+std::strlen(status),"  ");
      number(status+std::strlen(status),search.count());p.body(8,STATUS_Y,status,132);
      p.body(144,STATUS_Y,query.input().active_group(),32);
      if(query.input().caps())p.body(184,STATUS_Y,"Caps",48);else if(query.input().shift_armed())p.body(184,STATUS_Y,"Shift",48);
     }
    }
    p.flip();
   }
   release();
  }
 }
 bn::core::update();bn::core::update();renderer.reset();return picked;
}

bool run_dictionary_screen(Renderer& renderer,VocabFile* target,DictionaryResult& result,const char* preferred) {
 // Separate bounded index and canvas frames; no permanent EWRAM allocation.
 DictionarySearch search(additions);
 return dictionary_screen_inner(renderer,target,result,search,preferred);
}

bool dictionary_accept_pair(Renderer& renderer,VocabFile& vf,DictionaryResult& result) {
 if(vf.pair_blocked)return false;
 // V4.4: a result from a dictionary the list is not linked to asks before
 // linking it. V4.4.3: when the list's languages are unknown or others,
 // one sample card says which column is which (no "Set list languages").
 const bool link=result.dictionary[0]&&std::strcmp(vf.settings.dictionary,result.dictionary);
 const char* a=result.languages.front;const char* b=result.languages.back;
 bool accepted=true;
 const bool ask=link||!pair_fits(vf.languages,a,b);
 if(ask) {
  renderer.reset();bn::core::update();bn::core::update();
  Canvas p;
  if(link) {
   const char* lines[]={"List is not linked to this","dictionary.","Create link and go on?",""};
   static const char* const no_yes[]={"No","Yes"};
   accepted=run_choice(p,"Add from dictionary",lines,4,no_yes,2)==1;release();
  }
  if(accepted&&!pair_fits(vf.languages,a,b)) {
   char label[24];dict_build_label(a,label);
   int column=ask_column(p,vf,"Add from dictionary",label);
   if(column<0)accepted=false;else set_list_pair(vf,a,b,column);
  }
  if(accepted&&link){if(vf.settings.prefer(result.dictionary))vf.pair_dirty=true;else accepted=false;}
 }
 // The result goes into the list's columns: swapped when the list is b / a.
 if(accepted&&!std::strcmp(vf.languages.front,b)&&!std::strcmp(vf.languages.back,a)) {
  char tmp[192];std::strcpy(tmp,result.front);std::strcpy(result.front,result.back);std::strcpy(result.back,tmp);
  char code[12];std::strcpy(code,result.languages.front);std::strcpy(result.languages.front,result.languages.back);std::strcpy(result.languages.back,code);
 }
 if(!ask)return accepted;   // nothing was shown: the learning screen stays as it is
 bn::core::update();bn::core::update();renderer.reset();return accepted;
}

bool dictionary_link(Renderer& renderer,VocabFile& vf,bool choose) {
 bool linked=false;
 renderer.reset();bn::core::update();bn::core::update();
 {
  DictionaryCatalog catalog(vocab_file_sd_ready(),vocab_file_dictionary_fil(),vocab_file_dictionary_opened());
  DictionaryChoices choices(catalog);
  int current=choices.find(vf.settings.dictionary);
  choices.mark(vf.settings.dictionary);
  Canvas p;
  if(!choose&&current>=0) {
   // Linked already; lists linked before V4.4.3 may still need languages.
   linked=dictionary_languages(p,vf,catalog.dictionary(choices.index(current)),"Add from dictionary");
  } else if(!choose&&!choices.size()) {
   if(catalog.has_legacy())notice(p,"Upgrade .dict with PC Save As","Use PC builder 2.0 or newer.");
   else notice(p,"No valid .dict in /gbavocab.","Copy .dict from the PC builder.");
  } else {
   // Entry editor row: the dictionaries, then Type codes (languages without
   // a dictionary; the only row when no .dict is on the card).
   static const char* const type_codes[]={"Type codes"};
   if(choose)choices.extras(type_codes,1);
   if(choose&&!choices.size()&&catalog.has_legacy())notice(p,"Upgrade .dict with PC Save As","Use PC builder 2.0 or newer.");
   const char* title=!choices.size()?"Languages":current>=0?"Link to new Dictionary":"Link to Dictionary";
   int pick=0;
   // One dictionary on the card (Add from dictionary): linked without asking.
   if(choose||choices.size()>1){
    entries::Nav nav(choices.count());if(current>=0)nav.select(uint32_t(current));
    pick=run_entries(p,title,choices,nav,entries_row_cache())==EntriesResult::open?int(nav.sel()):-1;
    release();
   }
   if(pick>=choices.size()) {
    if(type_list_codes(p,vf)&&current>=0) {
     // A linked dictionary of other languages no longer fits the list.
     auto d=catalog.dictionary(choices.index(current));
     if(!pair_fits(vf.languages,d.code(0),d.code(1))){vf.settings.dictionary[0]=0;vf.pair_dirty=true;}
    }
   } else if(pick>=0) {
    int i=choices.index(pick);const char* name=catalog.filename(i);
    if(dictionary_languages(p,vf,catalog.dictionary(i),title,choose)) {
     if(std::strcmp(name,vf.settings.dictionary)&&vf.settings.prefer(name))vf.pair_dirty=true;
     linked=true;
    }
   }
   release();
  }
 }
 bn::core::update();bn::core::update();renderer.reset();return linked;
}

void dictionary_new_list(Renderer& renderer,VocabFile& vf) {
 renderer.reset();bn::core::update();bn::core::update();
 {
  DictionaryCatalog catalog(vocab_file_sd_ready(),vocab_file_dictionary_fil(),vocab_file_dictionary_opened());
  DictionaryChoices choices(catalog);
  static const char* const more[]={"Type codes","Later"};
  choices.extras(more,2);
  Canvas p;entries::Nav nav(choices.count());
  for(;;) {
   int pick=run_entries(p,"List languages",choices,nav,entries_row_cache())==EntriesResult::open?int(nav.sel()):-1;
   release();
   if(pick<0||pick==choices.size()+1)break;              // B or Later
   if(pick==choices.size()){if(type_list_codes(p,vf))break;continue;}
   int i=choices.index(pick);
   if(dictionary_languages(p,vf,catalog.dictionary(i),"List languages")){
    if(vf.settings.prefer(catalog.filename(i)))vf.pair_dirty=true;
    break;
   }
  }
  release();
 }
 bn::core::update();bn::core::update();renderer.reset();
}

bool dictionary_available() {
 DictionaryCatalog catalog(vocab_file_sd_ready(),vocab_file_dictionary_fil(),vocab_file_dictionary_opened());
 for(int i=0;i<catalog.count();++i)if(catalog.dictionary(i).valid())return true;
 return false;
}

bool dictionary_pick(Renderer& renderer,char out[64],bool& single) {
 bool picked=false;single=false;
 renderer.reset();bn::core::update();bn::core::update();
 {
  DictionaryCatalog catalog(vocab_file_sd_ready(),vocab_file_dictionary_fil(),vocab_file_dictionary_opened());
  DictionaryChoices choices(catalog);
  choices.mark(last_dictionary);   // the dictionary opened last
  Canvas p;
  if(!choices.size()){
   if(catalog.has_legacy())notice(p,"Upgrade .dict with PC Save As","Use PC builder 2.0 or newer.");
   else notice(p,catalog.count()?"No valid .dict in /gbavocab.":"No .dict in /gbavocab yet.","Import one with Start on home.");
  } else if(choices.size()==1){
   std::strcpy(out,catalog.filename(choices.index(0)));picked=single=true;
  } else {
   entries::Nav nav(choices.count());int last=choices.find(last_dictionary);nav.select(uint32_t(last>=0?last:0));
   if(run_entries(p,"Dictionaries",choices,nav,entries_row_cache())==EntriesResult::open){
    std::strcpy(out,catalog.filename(choices.index(int(nav.sel()))));picked=true;
   }
   release();
  }
 }
 bn::core::update();bn::core::update();renderer.reset();return picked;
}
