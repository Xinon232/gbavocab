#pragma once
#include <cstring>
struct PairMetadata {
    static bool starts(const char* s,const char* prefix) {
        while(*prefix)if(*s++!=*prefix++)return false;
        return true;
    }
    char front[12]={},back[12]={};
    bool present() const {return front[0]&&back[0];}
    bool same(const PairMetadata& p) const {return !std::strcmp(front,p.front)&&!std::strcmp(back,p.back);}
    static bool code(const char* s) {
        auto n=std::strlen(s);if(!n||n>11||s[0]<'a'||s[0]>'z')return false;
        for(unsigned i=0;i<n;++i)if(!((s[i]>='a'&&s[i]<='z')||(s[i]>='0'&&s[i]<='9')||s[i]=='-'))return false;
        return true;
    }
    bool set(const char* a,const char* b) {
        if(!code(a)||!code(b)||!std::strcmp(a,b))return false;
        std::strcpy(front,a);std::strcpy(back,b);return true;
    }
    bool parse(const char* row) {
        const char* prefix="# gbavocab: front=";
        if(!starts(row,prefix))return false;
        const char* a=row+std::strlen(prefix);const char* divider=std::strchr(a,';');
        if(!divider||divider-a>11||!starts(divider,"; back=")||std::strlen(divider+7)>11)return false;
        char f[12]={};std::memcpy(f,a,divider-a);
        return set(f,divider+7);
    }
    // Caller supplies 64 bytes. Footer is a final non-entry line.
    int format(char out[64]) const {
        out[0]=0;if(!present())return 0;
        std::strcpy(out,"# gbavocab: front=");std::strcpy(out+std::strlen(out),front);
        std::strcpy(out+std::strlen(out),"; back=");std::strcpy(out+std::strlen(out),back);std::strcpy(out+std::strlen(out),"\r\n");
        return int(std::strlen(out));
    }
};

// Appends at most `limit` bytes of `text` (ASCII codes; no strncat on the GBA).
inline void append_limited(char* out,const char* text,std::size_t limit) {
    std::size_t n=std::strlen(out);
    for(std::size_t i=0;i<limit&&text[i];++i)out[n++]=text[i];
    out[n]=0;
}

// V4.4.3 learning header: the asked language first ("de > en"), mixed mode
// adds " (mixed)" for the current card; "Front > Back" without codes. Codes
// may have 11 characters; 5 each keep the header within 4 UI sprites.
inline void direction_label(const PairMetadata& languages,bool front_first,bool mixed,char out[48]) {
    const bool codes=languages.present();
    const char* first=codes?languages.front:"Front";
    const char* second=codes?languages.back:"Back";
    if(!front_first){const char* t=first;first=second;second=t;}
    const std::size_t limit=codes?5:12;
    out[0]=0;append_limited(out,first,limit);std::strcat(out," > ");append_limited(out,second,limit);
    if(mixed)std::strcat(out," (mixed)");
}
