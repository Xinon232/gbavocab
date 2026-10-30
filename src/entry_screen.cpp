#include "entry_screen.h"
#include "entry_render.h"
#include "entries_screen.h"
#include "dictionary_screen.h"
#include "bn_core.h"
#include "bn_keypad.h"

namespace {
BN_DATA_EWRAM_BSS EntryEditor editor(entry_glyph_width);
uint16_t keys() {
    // Exact GBAWriter Button ordering, not the GBA hardware KEYINPUT ordering.
    const bool held[]={bn::keypad::up_held(),bn::keypad::down_held(),bn::keypad::left_held(),bn::keypad::right_held(),bn::keypad::a_held(),bn::keypad::b_held(),bn::keypad::l_held(),bn::keypad::r_held(),bn::keypad::start_held(),bn::keypad::select_held()};
    uint16_t result=0;for(unsigned i=0;i<10;++i)if(held[i])result|=1u<<i;
    return result;
}
void release(){while(keys())bn::core::update();}
void ui_line(void* context,int x,int y,const char* text){auto& c=*static_cast<Canvas*>(context);if(x<0)c.ui_center(y,text);else c.ui(x,y,text);}
bool list_needs_save(const VocabFile& vf){return vocab_any_dirty(vf)||vf.array_generation||vf.pair_dirty;}

// List entries: the rows in file order (after the save, the TXT order), one
// unselectable divider before each non-empty box.
class ListEntries final : public EntriesSource {
public:
    ListEntries(VocabFile& vf,const char* fallback,int used):vf_(vf),fallback_(fallback),used_(used){rebuild();}
    void rebuild(){
        dividers_=0;
        for(int i=0;i<vf_.line_count;++i){
            if(i&&vf_.field[i]==vf_.field[i-1])continue;
            if(dividers_==MAX_DIVIDERS){dividers_=0;return;}   // not grouped by box: no dividers
            at_[dividers_]=uint32_t(i+dividers_);box_[dividers_]=vf_.field[i];++dividers_;
        }
    }
    uint32_t count() override {return uint32_t(vf_.line_count+dividers_);}
    bool divider(uint32_t row) override {for(int k=0;k<dividers_;++k)if(at_[k]==row)return true;return false;}
    int entry(uint32_t row) const {int before=0;for(int k=0;k<dividers_;++k)if(at_[k]<row)++before;return int(row)-before;}
    uint32_t row_of(int entry) const {uint32_t row=uint32_t(entry);for(int k=0;k<dividers_;++k)if(at_[k]<=row)++row;return row;}
    bool text(uint32_t row,char out[192]) override {
        for(int k=0;k<dividers_;++k)if(at_[k]==row){
            char label[48];entries_box_label(k?box_[k-1]:0,box_[k],label);std::strcpy(out,label);return true;
        }
        char raw[VOCAB_RAW_LINE_MAX];
        if(!vocab_file_raw_row(vf_,fallback_,used_,entry(row),raw))return false;
        char* tab=std::strchr(raw,'\t');if(!tab)return false;
        *tab=0;char* back=tab+1;char* extra=std::strchr(back,'\t');if(extra)*extra=0;
        const char* first=swapped_?back:raw;const char* second=swapped_?raw:back;
        // One line of at most 191 bytes: a maximum-length word keeps priority.
        unsigned a=std::strlen(first),b=std::strlen(second);
        if(a>188)a=188;
        if(a+3+b>191)b=191-3-a;
        while(a&&(first[a]&0xc0)==0x80)--a;          // never cut a UTF-8 character
        while(b&&(second[b]&0xc0)==0x80)--b;
        std::memcpy(out,first,a);std::memcpy(out+a," = ",3);std::memcpy(out+a+3,second,b);out[a+3+b]=0;
        return true;
    }
    bool swap(uint32_t&) override {swapped_=!swapped_;return true;}
    static bool skip(void* self,uint32_t row){return static_cast<ListEntries*>(self)->divider(row);}
private:
    static constexpr int MAX_DIVIDERS=5;
    VocabFile& vf_;const char* fallback_;int used_;
    int dividers_=0;uint32_t at_[MAX_DIVIDERS]={};uint8_t box_[MAX_DIVIDERS]={};bool swapped_=false;
};

// V4.4: added, edited and deleted entries are saved at once instead of
// waiting in RAM for Start. The save rewrites the TXT and rereads it, so it
// cannot run behind the learning screen (which reads cards from the same
// file); "save..." stays in front until it is done. False: vocab_file_last_error().
bool autosave(Canvas& c,State& state){
    if(!vocab_file_loaded_from_sd()||!vocab_file_save_after_changes())return true;
    entries_show_saving(c);
    return app_save_active_list(state);
}
void autosave_or_notice(Canvas& c,State& state){
    if(autosave(c,state))return;
    const char* lines[]={"Not saved:",vocab_file_last_error(),"","The change is kept; Start saves","the list again."};
    run_notice(c,"Saving list",lines,5);
    release();
}

// The usual two-step entry draft for one row, as Edit entry in the menu.
bool edit_list_entry(Canvas& c,State& state,VocabFile& vf,int index,const char* raw){
    EntryEditorLoan loan(editor);
    editor.open(index,raw);
    if(!editor.begin_edit())return false;
    while(editor.active()&&editor.screen()!=EntryEditor::Screen::menu){
        editor.frame(keys());
        if(editor.commit_requested()){
            {const char* lines[]={"Applying entry..."};draw_message(c,editor.heading(),lines,1);}
            int new_index=index;
            bool ok=vocab_file_defer(vf,EntryMutation::edit,index,editor.row(),new_index);
            if(ok)state.entry_committed(vf,new_index);
            editor.finish(ok,vocab_file_last_error());
            if(ok){autosave_or_notice(c,state);return true;}
        }
        c.clear();
        if(editor.active())render_entry(editor,c.pixels,ui_line,&c);
        c.flip();
    }
    return false;
}

void run_list_entries(State& state,VocabFile& vf,const char* fallback,int used){
    Canvas c;
    ListEntries source(vf,fallback,used);
    entries::Nav nav(source.count(),ListEntries::skip,&source);
    // The learning card's entry is selected first.
    int current=state.entry_target(vf);
    if(current>=0)nav.select(source.row_of(current));
    static const char* const items[]={"Edit entry","Delete entry"};
    static const char* const confirm[]={nullptr,"Delete entry? A: Yes  B: No"};
    while(run_entries(c,"List entries",source,nav,entries_row_cache())==EntriesResult::open){
        int index=source.entry(nav.sel());
        char raw[VOCAB_RAW_LINE_MAX];
        if(!vocab_file_raw_row(vf,fallback,used,index,raw))continue;
        int choice=run_menu(c,"List entry",items,confirm,2);
        if(choice==0)edit_list_entry(c,state,vf,index,raw);
        else if(choice==1){
            int result=index;
            if(vocab_file_defer(vf,EntryMutation::remove,index,nullptr,result)){state.entry_committed(vf,result);autosave_or_notice(c,state);}
            source.rebuild();nav.resize(source.count());
            if(vf.line_count)nav.select(source.row_of(index<vf.line_count?index:vf.line_count-1));
        }
        release();
    }
    release();
}
}
EntryEditor& entry_draft_editor(){return editor;}
void run_entry_screen(Renderer& renderer,State& state,VocabFile& vf,const char* fallback,int used,const char* front,const char* back) {
    auto reopen=[&](){
        const int target=state.entry_target(vf);
        char raw[VOCAB_RAW_LINE_MAX];
        const bool readable=vocab_file_raw_row(vf,fallback,used,target,raw);
        editor.open(readable?target:-1,readable?raw:nullptr);
    };
    // V4.4.3: languages and Type first for labels; the link row reads
    // "Languages" when no .dict is on the card.
    bool dictionaries=dictionary_available();
    auto languages=[&](){editor.set_languages(vf.languages,vf.settings.type_first==1,dictionaries);};
    reopen();languages();
    if(front&&back)editor.prefill_add(front,back);
    renderer.reset();
    // Release deferred sprite tiles before changing into bitmap mode's smaller
    // OBJ window, and again before recreating learning-screen sprites on exit.
    bn::core::update();bn::core::update();
    while(editor.active()) {
      bool dictionary_requested=false;
      auto request=EntryEditor::MenuRequest::none;
      {
        Canvas c;
        while(editor.active()) {
            editor.set_list_state(list_needs_save(vf),vf.settings.dictionary[0]!=0);
            editor.frame(keys());
            if(editor.take_dictionary_request()){dictionary_requested=true;break;}
            request=editor.take_menu_request();
            if(request!=EntryEditor::MenuRequest::none)break;
            if(editor.commit_requested()) {
                {const char* lines[]={"Applying entry..."};draw_message(c,editor.heading(),lines,1);}
                int new_index=editor.target();
                bool saved=vocab_file_defer(vf,editor.operation(),editor.target(),editor.row(),new_index);
                bool installed=saved;
                if(installed)state.entry_committed(vf,new_index);
                editor.finish(installed,vocab_file_last_error());
                if(installed&&!autosave(c,state)) {
                    // Back in learning, the failure shows like a failed Start save.
                    renderer.set_save_error(vocab_file_last_error());
                    renderer.set_save_status(SaveStatus::FAILED);
                }
            }
            if(editor.active()&&editor.screen()==EntryEditor::Screen::menu) {
                // gbamp3 list look, like every other menu; a message (for
                // example a failed save) is the grey bottom line.
                const char* items[EntryEditor::MENU_ROWS];
                for(int i=0;i<EntryEditor::MENU_ROWS;++i)items[i]=entry_menu_item(editor,i);
                entries_draw_menu(c,editor.heading(),items,EntryEditor::MENU_ROWS,editor.selection(),editor.message());
            } else {
                c.clear();
                if(editor.active())render_entry(editor,c.pixels,ui_line,&c);
            }
            c.flip();
        }
      }
      bn::core::update();bn::core::update();
      using R=EntryEditor::MenuRequest;
      if(request==R::save_list_entries||request==R::list_entries){
        bool ok=true;
        if(request==R::save_list_entries){
            {
                // The menu stays on screen with "save..." in front until saved.
                Canvas c;
                const char* items[EntryEditor::MENU_ROWS];
                for(int i=0;i<EntryEditor::MENU_ROWS;++i)items[i]=entry_menu_item(editor,i);
                entries_draw_menu(c,editor.heading(),items,EntryEditor::MENU_ROWS,editor.selection(),nullptr);
                c.flip();
                entries_show_saving(c);
                ok=app_save_active_list(state);
            }
            // Butano frees a bitmap background's VRAM only on the next frames:
            // without these updates the list's canvas cannot be allocated
            // ("All affine BG tiles allocate failed").
            bn::core::update();bn::core::update();
        }
        if(ok)run_list_entries(state,vf,fallback,used);
        bn::core::update();bn::core::update();
        reopen();editor.select_row(EntryEditor::ROW_LIST);
        if(!ok)editor.set_message(vocab_file_last_error());
      } else if(request==R::link){
        dictionary_link(renderer,vf,true);
        if(vf.pair_dirty&&!vocab_file_save_settings(vf))editor.set_message(vocab_file_last_error());
        dictionaries=dictionary_available();languages();
        editor.select_row(EntryEditor::ROW_LINK);
      } else if(request==R::type_first){
        // V4.4.3 Type first: saved in GBAVOCAB.SYS at once, like the link.
        vf.settings.type_first^=1;vf.pair_dirty=true;
        if(!vocab_file_save_settings(vf))editor.set_message(vocab_file_last_error());
        languages();editor.select_row(EntryEditor::ROW_TYPE_FIRST);
      }
      if(dictionary_requested){
        if(dictionary_link(renderer,vf,false)){
            if(vf.pair_dirty)vocab_file_save_settings(vf);
            DictionaryResult result;
            if(run_dictionary_screen(renderer,&vf,result)&&dictionary_accept_pair(renderer,vf,result))
                editor.prefill_add(result.front,result.back);
        }
        if(vf.pair_dirty&&!vocab_file_save_settings(vf))editor.set_message(vocab_file_last_error());
        languages();
      }
    }
    bn::core::update();bn::core::update();
    renderer.reset();
}
