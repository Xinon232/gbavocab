#pragma once
#include "vocab.h"
class Renderer;
struct DictionaryResult {
    PairMetadata languages;
    char front[VOCAB_RAW_LINE_MAX]={},back[VOCAB_RAW_LINE_MAX]={};
    char dictionary[64]={};   // the .dict the result came from
};
// preferred: a .dict filename to open directly (main menu > Dictionaries).
bool run_dictionary_screen(Renderer&,VocabFile* target,DictionaryResult&,const char* preferred=nullptr);
// Main menu > Dictionaries: every valid .dict (name and languages). With one
// dictionary it is chosen without a screen (single). False on B / none.
bool dictionary_pick(Renderer&,char out[64],bool& single);
// Ask missing FRONT/BACK orientation, or verify/map existing footer. No writes.
bool dictionary_accept_pair(Renderer&,VocabFile&,DictionaryResult&);
// Make sure the list has a linked dictionary (V4.0): an existing valid link
// is kept; with one dictionary on the card it is linked at once; otherwise a
// picker opens. choose: always open the picker (Link to new Dictionary).
// Changes set vf.pair_dirty; the caller saves the settings. No TXT writes.
bool dictionary_link(Renderer&,VocabFile& vf,bool choose);
// V4.4.3: true when /gbavocab holds at least one usable .dict.
bool dictionary_available();
// V4.4.3 New List: "List languages" with every dictionary (links it and
// takes its languages), Type codes, and Later. Sets vf.pair_dirty.
void dictionary_new_list(Renderer&,VocabFile& vf);
