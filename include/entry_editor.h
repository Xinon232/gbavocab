#pragma once
#include "writer_core.h"
#include "writer_layout.h"
#include "vocab_file_io.h"
#include "pair_metadata.h"

class EntryEditor {
public:
    enum class Screen { closed, menu, front, back, confirm_delete };
    explicit EntryEditor(writer::Layout::Width measure) : measure_(measure) {}
    void open(int target, const char* raw);
    bool prefill_add(const char* front,const char* back);
    bool open_dictionary_mutation(EntryMutation operation,const char* front,const char* back);
    void open_lookup(bool allow_add=false);
    enum class LookupAction { none, up, down, direction, chooser, select, cancel, add };
    LookupAction take_lookup_action() {auto a=lookup_action_;lookup_action_=LookupAction::none;return a;}
    bool take_dictionary_request() {bool r=dictionary_request_;dictionary_request_=false;return r;}
    // V4.0 menu rows. Show list entries asks "Saving list first. OK?" in its
    // row when the list has unsaved changes; Link to (new) Dictionary.
    // V4.4.3: Type first (A switches which column step 1/2 types).
    static constexpr int MENU_ROWS = 7, ROW_LIST = 4, ROW_LINK = 5, ROW_TYPE_FIRST = 6;
    enum class MenuRequest { none, list_entries, save_list_entries, link, type_first };
    MenuRequest take_menu_request() {auto r=menu_request_;menu_request_=MenuRequest::none;return r;}
    void set_list_state(bool needs_save,bool linked) {needs_save_=needs_save;linked_=linked;}
    // V4.4.3 list languages: the codes ("" while unknown), Type first (true:
    // step 1/2 types the back column) and whether a .dict is on the card.
    void set_languages(const PairMetadata& codes,bool back_first,bool dictionaries) {
        languages_=codes;back_first_=back_first;dictionaries_=dictionaries;
    }
    const PairMetadata& languages() const { return languages_; }
    bool back_first() const { return back_first_; }
    bool dictionaries() const { return dictionaries_; }
    // The column (0 front, 1 back) the current step types, and its label.
    int column() const { return (screen_ == Screen::back ? 1 : 0) ^ (back_first_ ? 1 : 0); }
    const char* field_label() const;
    bool asking_save() const { return asking_save_; }
    bool linked() const { return linked_; }
    // Edit draft of the opened target (List entries route); false: not editable.
    bool begin_edit();
    void select_row(int row) { selection_ = row; }
    void set_message(const char* text) { message_ = text; }
    void frame(uint16_t held);
    void finish(bool committed, const char* error);
    Screen screen() const { return screen_; }
    bool autosave() const { return false; }
    bool active() const { return screen_ != Screen::closed; }
    int selection() const { return selection_; }
    int target() const { return target_; }
    bool commit_requested() const { return commit_; }
    EntryMutation operation() const { return operation_; }
    const char* row() const { return row_; }
    const char* captured() const { return captured_; }
    const char* message() const { return message_; }
    writer::TextModel& text() { return drafts_[column()]; }
    writer::Layout& layout() { return layout_; }
    int viewport() const { return viewport_; }
    int view_rows() const { return status_visible_ || message_[0] ? 5 : 6; }
    bool status_visible() const { return status_visible_; }
    bool caret_visible() const { return clock_.visible(); }
    const writer::InputState& input() const { return input_; }
    const char* heading() const;
private:
    void consume(writer::InputEvent event);
    void change(Screen screen);
    void reflow();
    bool prepare_row();
    writer::Layout::Width measure_;
    writer::TextModel drafts_[2];
    writer::InputState input_;
    writer::Layout layout_;
    writer::CaretClock clock_;
    Screen screen_ = Screen::closed;
    EntryMutation operation_ = EntryMutation::add;
    uint16_t previous_ = 0;
    bool wait_release_ = true, commit_ = false, status_visible_ = true;
    bool dictionary_request_=false,lookup_=false,lookup_add_=false;
    MenuRequest menu_request_=MenuRequest::none;
    bool needs_save_=false,linked_=false,asking_save_=false;
    LookupAction lookup_action_=LookupAction::none;
    uint16_t lookup_once_=0;
    bool start_alone_=false;
    PairMetadata languages_;
    bool back_first_=false,dictionaries_=true;
    // Step 1/2 <-> 2/2 with Start+A / Start+B: Start may stay held.
    bool keep_start_=false;
    bool provisional_ = false, provisional_dirty_ = false;
    std::size_t provisional_end_ = 0;
    int selection_ = 0, target_ = -1, viewport_ = 0;
    char captured_[VOCAB_RAW_LINE_MAX] = {}, row_[VOCAB_RAW_LINE_MAX] = {};
    char suffix_[VOCAB_RAW_LINE_MAX] = {};
    const char* message_ = "";
};

// Stack-only exclusive loan. Layout and TextModel own their storage.
#include <type_traits>
#include <cstring>
class EntryEditorLoan {
public:
    explicit EntryEditorLoan(EntryEditor& editor):editor_(editor) {
        static_assert(std::is_trivially_copyable<EntryEditor>::value,"editor loan needs owning value state");
        std::memcpy(saved_,&editor_,sizeof editor_);
    }
    ~EntryEditorLoan(){std::memcpy(&editor_,saved_,sizeof editor_);}
    EntryEditorLoan(const EntryEditorLoan&)=delete;
    EntryEditorLoan& operator=(const EntryEditorLoan&)=delete;
private:
    EntryEditor& editor_;
    unsigned char saved_[sizeof(EntryEditor)];
};
