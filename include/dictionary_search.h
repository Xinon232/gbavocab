#pragma once
#include "dictionary.h"
#include "dictionary_additions.h"
// Live overlay results precede base matches. Removed base ranks are a bounded
// sparse index, never a base-sized bitmap or a linear base scan.
class DictionarySearch {
public:
 explicit DictionarySearch(DictionaryAdditions& saved):saved_(saved){}
 void open(Dictionary d){dictionary_=d;last_valid_=false;saved_.open(dictionary_);search(0,"");}
 void search(int side,const char* prefix){
  // Typing one more character can only narrow the previous base range, so
  // search inside it. Only a valid UTF-8 previous query is a safe fold prefix.
  const Dictionary::Range* within=nullptr;unsigned n=std::strlen(prefix);
  if(last_valid_&&side==side_&&last_n_<=n&&!std::memcmp(last_,prefix,last_n_))within=&range_;
  side_=side;range_=dictionary_.prefix(side,prefix,within);
  last_valid_=!dictionary_.failed()&&n<sizeof last_&&writer::valid_utf8(prefix,n);
  if(last_valid_){std::memcpy(last_,prefix,n+1);last_n_=n;}
  int found=saved_.search(side,prefix,suppressed_,&removed_);overlay_failed_=found<0;added_=found<0?0:unsigned(found);
  if(found<0)removed_=0;
  // No partial v2 view: a failed base bound must not underflow the sparse
  // subtraction, expose stale records, or look like an empty successful query.
  if(dictionary_.failed()||(found<0&&dictionary_.editable())){range_={};removed_=added_=0;last_valid_=false;}
 }
 bool failed() const{return dictionary_.failed()||overlay_failed_;}
 // Whole-list view (Dictionary list entries): positions [0, added()) are the
 // live additions/edits, the rest the visible base rows in sorted order.
 unsigned added() const{return added_;}
 int side() const{return side_;}
 const Dictionary& dictionary() const{return dictionary_;}
 // Visible base rows sorted before base rank r (removed rows not counted).
 uint32_t visible_before(uint32_t r) const{uint32_t n=0;for(unsigned i=0;i<removed_;++i)if(suppressed_[i]<r)++n;return r-n;}
 uint32_t count() const{return added_+range_.end-range_.begin-removed_;}
 uint32_t identity(uint32_t position){
  if(position>=count())return ~0u;
  if(position<added_)return saved_.identity(position);
  return dictionary_.row_at(side_,base_rank(position));
 }
 bool read(uint32_t position,char front[192],char back[192]){
  if(position>=count())return false;
  if(position<added_){char row[192];if(!saved_.read(position,row))return false;char* t=std::strchr(row,'\t');if(!t)return false;*t=0;std::strcpy(front,row);std::strcpy(back,t+1);}
  else if(!dictionary_.read_rank(side_,base_rank(position),front,back))return false;
  return true;
 }
private:
 uint32_t base_rank(uint32_t position) const {
  uint32_t rank=range_.begin+position-added_;for(unsigned i=0;i<removed_;++i)if(suppressed_[i]<=rank)++rank;else break;
  return rank;
 }
 DictionaryAdditions& saved_;Dictionary dictionary_;Dictionary::Range range_;
 uint32_t suppressed_[dict_format::LIMIT];unsigned added_=0,removed_=0;int side_=0;bool overlay_failed_=false;
 char last_[192]={};unsigned last_n_=0;bool last_valid_=false;
};
