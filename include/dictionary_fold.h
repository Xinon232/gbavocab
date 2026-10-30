#pragma once
#include "dictionary_accent_table.h"
namespace dictionary_fold {
// Streaming scalar iterator: no normalized-key buffers or persistent RAM.
// For valid UTF-8 scalar ordering equals unsigned UTF-8 byte ordering.
class Iterator {
 const unsigned char* p_; bool latin_=false;
 unsigned decode(){
  unsigned a=*p_;if(!a)return 0;++p_;if(a<128)return a;
  unsigned n=a>=0xc2&&a<=0xdf?1:a>=0xe0&&a<=0xef?2:a>=0xf0&&a<=0xf4?3:0;
  if(!n)return 0x110000+a;
  unsigned cp=a&((1u<<(6-n))-1);
  for(unsigned i=0;i<n;++i){unsigned b=p_[i];if(b<0x80||b>0xbf)return 0x110000+a;cp=(cp<<6)|(b&63);}
  if(cp<(n==1?0x80u:n==2?0x800u:0x10000u)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return 0x110000+a;
  p_+=n;return cp;
 }
public:
 explicit Iterator(const char* p):p_(reinterpret_cast<const unsigned char*>(p)){}
 unsigned next(){
  for(;;){unsigned c=decode();if(latin_&&c>=0x300&&c<=0x36f)continue;
   unsigned lo=0,hi=sizeof(mappings)/sizeof(mappings[0]);
   if(c>=0xc0&&c<=0x1eff){while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(mappings[mid].from<c)lo=mid+1;else hi=mid;}
    if(lo<sizeof(mappings)/sizeof(mappings[0])&&mappings[lo].from==c)c=mappings[lo].to;}
   if(c>='A'&&c<='Z')c+=32;
   latin_=c>='a'&&c<='z';return c;
  }
 }
};
inline int compare(const char* word,const char* query,bool prefix=false){
 Iterator a(word),b(query);for(;;){unsigned y=b.next();if(prefix&&!y)return 0;unsigned x=a.next();if(x!=y)return x<y?-1:1;if(!x)return 0;}
}
}
