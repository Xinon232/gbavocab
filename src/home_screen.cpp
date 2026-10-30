#include "home_screen.h"

namespace {
struct TextPage { const char* heading; const char* lines[HOME_PAGE_LINES]; };
// Pages grouped by topic of the Settings list (Select on the home screen), most useful first.
// gbareader About layout: one row per line, "#" lines are grey subheadings.
constexpr TextPage help[HOME_HELP_PAGES] = {
    // Quick start
    {"Files", {"#Where files go", "Lists (TXT) and dictionaries", "(.dict) go in /gbavocab.", "Main menu Start: Import.", "#Lists", "TXT row: word TAB translation;", "an empty line starts the next box", "(dict.cc format, Box 1 to 5)."}},
    {"Controls", {"#Learning", "Hold R: answer. A: right. B: wrong.", "Start: save the list.", "Select: main menu.", "Start+Select: Entry editor.", "#Main menu", "Select: Settings. Start: Import.", "B: resume list. L+R: sleep."}},
    {"Main menu", {"Dictionaries first, then your", "lists A to Z. A: open.", "L + Up/Down: next letter.", "Play mark: open list; B: resume.", "#Other keys", "Select: Settings (New List).", "Start: Import.", "L+R sleeps. Start held 1 s wakes."}},
    {"Typing: get GBAWriter", {"Words are typed like in", "GBAWriter (D-pad + B / A / R).", "Highly recommended: learn it", "with GBAWriter first.", "#Get it", "github.com/Xinon232/gbawriter", "", "Details: Settings > Typing."}},
    // Learning
    {"Learning", {"#Cards", "Hold R: show the answer.", "A: right, card moves a box up.", "B: wrong, card goes to Box 1.", "#Boxes and modes", "Left/Right: Box 1 to 5.", "Up: undo; Down: shuffle box.", "L: asked language (de > en)."}},
    {"Learning screen", {"#Top", "Asked language (de > en);", "Box N/5 - T words: box shown,", "words in the list.", "#Bottom", "Words per box; the line marks", "the box shown.", "Hide: Settings > Hide elements."}},
    {"Saving and switching", {"#Saving", "Start: save the list. New words", "are saved at once (save...).", "#Switching", "Select: main menu (saves first).", "Start+Select: Entry editor.", "Settings and the last list are", "kept in /gbavocab/GBAVOCAB.SYS."}},
    // Entry editor
    {"Entry editor", {"#Start+Select in learning", "Add, Edit, Delete entry:", "the card on screen.", "Add from dictionary.", "Show list entries.", "#Link to Dictionary", "The .dict and list languages.", "Type first: step 1/2 language."}},
    {"Entry drafts", {"#Two steps", "1/2: word; 2/2: translation.", "Start+A: next, then apply.", "Start+B: back or cancel.", "Both fields need text.", "#Limits", "191 bytes per row,", "15,000 entries per list."}},
    // List entries
    {"List entries", {"Every entry in file order;", "lines show where boxes change.", "Unsaved lists are saved first.", "#Keys", "Up/Down: move; Left/Right: page.", "Start: other language first.", "A: Edit / Delete entry.", "B: back."}},
    // Dictionary
    {"Dictionary", {"#Open", "Main menu: Dictionaries, or", "Entry editor: Add from dictionary.", "#Search", "Type a word: matches appear.", "Hold Start + Up/Down: choose.", "Start+A: take it  Start+B: back", "Start alone: other language."}},
    {"Dictionary menu", {"#Start+Select", "Add entry, Edit entry,", "Delete entry and", "Show Dictionary List entries.", "Edit / Delete: selected result.", "Changes are saved in the .dict.", "#Other dictionaries", "Start+R: next dictionary."}},
    {"Dictionary list entries", {"Whole dictionary A to Z.", "#Keys", "Up/Down: move; Left/Right: page.", "L + Up/Down: next letter.", "Start: other language first.", "A: Edit / Delete entry.", "B: back to the search.", ""}},
    {"Old .dict files", {"This ROM needs v4 .dict files.", "Open older ones in PC builder", "2.0 or newer, Save As new file.", "#Changes on the GBA", "A .dict takes 512 changes;", "PC Save As makes room again.", "", ""}},
    // Typing
    {"Typing letters", {"Hold a direction, then", "B/A/R: letter 1/2/3.", "#Letters", "Up: abc    Right: hij", "Down: nop  Left: tuw", "#Two more", "Hold Right, tap R twice: g.", "Hold Left, tap R twice: v."}},
    {"Typing / L layer", {"Hold L and a direction:", "#Letters", "Up: def    Right: klm", "Down: qrs  Left: xyz", "B/A/R: letter 1/2/3.", "", "", ""}},
    {"Spaces / case", {"#Spaces", "A alone: space; B: delete.", "Hold A/B alone: repeat.", "#Case", "Short R alone: Shift", "(next letter upper case).", "Hold R alone (~0.8 sec):", "Caps while held."}},
    {"Symbols", {"Select: new period.", "Hold Select to replace it:", "#While Select is held", "Up/Down: 1234567890", "Right/Left: .()/;@#%&_+=-", "R/L alone: .,'\":!?", "", ""}},
    {"Accents", {"#Two ways", "Hold Select, type a letter:", "it gets its first accent.", "Or type a letter, keep it", "held and press Select.", "#More accents", "Keep Select held; press B/A/R", "again for the next accent."}},
    {"Caret / status", {"#Caret", "Start+Left/Right: caret.", "Start+Up/Down: line.", "Start+L/R: page.", "Start+Select: status line.", "#Search", "Accents: manana finds mañana,", "cafe finds café."}},
    {"Arabic", {"Arabic in lists is shown with", "joined letters, right to left;", "Latin and digits stay LTR.", "Harakat are hidden, not lost.", "No Arabic typing layout.", "", "", ""}},
    // Import
    {"Import", {"#Main menu Start: browse card.", "Folders, TXT and .dict files.", "A: open / import; B: up.", "#What is copied", "TXT: import as list or", "as dictionary. .dict: copied.", "Same name: numbered copy.", "Nothing is replaced."}},
    {"Import as dictionary", {"Built on the GBA; large files", "take minutes: do not switch off.", "#Skipped rows", "Bad rows are skipped: too long,", "invalid and duplicates.", "#Languages", "Without a dict.cc header, type", "the two language codes."}}
};
struct Topic { const char* name; int first, pages; bool credits; };
constexpr Topic topics[HOME_HELP_TOPICS] = {
    {"New List", 0, 0, false}, {"Flashcard angle", 0, 0, false}, {"Save settings", 0, 0, false}, {"Hide elements", 0, 0, false},
    {"Quick start", 0, 4, false}, {"Learning", 4, 3, false}, {"Entry editor", 7, 2, false}, {"List entries", 9, 1, false},
    {"Dictionary", 10, 4, false}, {"Typing", 14, 7, false}, {"Import", 21, 2, false}, {"Credits", 0, HOME_CREDIT_PAGES, true}};
// gbareader's About order: the personal page first, then the license, then
// one page per part made by others with its license.
constexpr TextPage credits[HOME_CREDIT_PAGES] = {
    {"Credits", {"gbavocab 10.0", "(c) 2026 Halim Jarrar", "GPL-3.0-or-later", "No warranty.", "#Source:", "github.com/Xinon232/gbavocab", "halimj.itch.io", "gba@halim-jarrar.de"}},
    {"License", {"gbavocab is free software under", "the GNU GPL 3.0 or later", "(GPL-3.0-or-later), with", "no warranty; see the GPL.", "#Source code", "github.com/Xinon232/gbavocab", "Parts made by others and their", "licenses are on the next pages."}},
    {"Text and fonts", {"#Text renderer", "SuperFW by David Guillen Fandos", "License: GPL 3.0 or later", "#Fonts", "UNSCII by Viznut: GPL", "GNU Unifont: GPL 2.0 or later", "with the font embedding", "exception"}},
    {"Arabic font", {"Ghoulam Regular", "(C) 2025 Imad AlFil (mloukhiyye)", "License: CC BY 4.0", "", "Changed: converted to an 11 px", "bitmap font for gbavocab.", "#Source", "mloukhiyye.itch.io"}},
    {"SD card and files", {"#SD card driver: SuperCard", "SuperFW by David Guillen Fandos", "License: GPL 3.0 or later", "#Omega and EverDrive drivers", "Halim Jarrar (GPL), after the", "Omega kernel (Apache 2.0) and", "edio-gba by krikzz (MIT)", "Files: FatFs by ChaN (BSD style)"}},
    {"Engine", {"#Engine", "Butano by Gustavo Valiente", "License: zlib", "", "#Built with", "devkitPro / devkitARM", "", ""}},
    // Like gbamp3's last About page: A here opens Secret Settings.
    {"Secret Settings", {"Press A on this page.", "Letter Box: Off (default), On,", "Only L+DPAD.", "", "#Saving", "Settings (Select) >", "Save settings.", ""}}
};
}
const char* home_help_heading(int p) { return help[p].heading; }
const char* home_topic_name(int t) { return topics[t].name; }
int home_topic_first(int t) { return topics[t].first; }
int home_topic_pages(int t) { return topics[t].pages; }
bool home_topic_credits(int t) { return topics[t].credits; }
bool home_topic_new_list(int t) { return t == 0; }
bool home_topic_flashcard_angle(int t) { return t == 1; }
bool home_topic_save_settings(int t) { return t == 2; }
bool home_topic_hide_elements(int t) { return t == 3; }
const char* home_help_line(int p,int l) { return help[p].lines[l]; }
const char* home_credit_heading(int p) { return credits[p].heading; }
const char* home_credit_line(int p,int l) { return credits[p].lines[l]; }
// Secret Settings, under the line: what the Letter Box values do.
static const char* const secret_lines[HOME_SECRET_LINES] = {
    "Off: no letter box (default).", "On: box on L+Up/Down, pages,",
    "wraps and fast scrolling.", "Only L+DPAD: box on L+Up/Down."};
const char* home_secret_line(int l) { return secret_lines[l]; }
// Save settings, under the line: what the two switches do.
static const char* const save_lines[HOME_SAVE_LINES] = {
    "Save after entry changes: Add,", "Edit, Delete save the list now.",
    "Save list on menu: Select from", "learning saves the list first.",
    "Off: Start saves the list."};
const char* home_save_line(int l) { return save_lines[l]; }
// Flashcard angle, under the line.
static const char* const angle_lines[HOME_ANGLE_LINES] = {
    "Turns the learning screen:", "hold the GBA sideways or", "upside down. The D-pad turns",
    "with it; menus stay upright."};
const char* home_angle_line(int l) { return angle_lines[l]; }
// Hide elements, under the line.
static const char* const hide_lines[HOME_HIDE_LINES] = {
    "Hides parts of the learning", "screen; the cards and the rest",
    "stay where they are. Hidden", "boxes still work: Left/Right",
    "change the box as before."};
const char* home_hide_line(int l) { return hide_lines[l]; }

void HomeScreen::open_list(bool active, bool dirty, bool is_active) {
    if(request_ != Request::none) return;
    if(is_active) { request_ = Request::resume; return; }
    origin_ = Page::main; save_first_ = false;
    if(active && dirty) page_ = Page::confirm;
    else request_ = Request::load;
}

void HomeScreen::press(Key key, bool active, bool dirty) {
    if(request_ != Request::none) return;
    if(page_ == Page::confirm) {
        if(key == Key::select) page_ = origin_;
        else if(key == Key::a || key == Key::b) {
            save_first_ = key == Key::a;
            request_ = origin_ == Page::new_list ? Request::create : Request::load;
        }
        return;
    }
    if(page_ == Page::secret) {
        if(key == Key::b) page_ = Page::credits;
        return;
    }
    if(page_ == Page::save_settings || page_ == Page::flashcard_angle || page_ == Page::hide_elements) {
        if(key == Key::b) page_ = Page::help;
        return;
    }
    if(page_ == Page::controls || page_ == Page::credits) {
        const int first = topics[topic_].first, last = first + topics[topic_].pages - 1;
        if(key == Key::b) page_ = Page::help;
        else if(key == Key::a && page_ == Page::credits && help_page_ == last) page_ = Page::secret;
        else if(key == Key::left && help_page_ > first) --help_page_;
        else if(key == Key::right && help_page_ < last) ++help_page_;
        return;
    }
    if(page_ == Page::help) {
        if(key == Key::b) page_ = Page::main;
        else if(key == Key::up) topic_ = (topic_ + HOME_HELP_TOPICS - 1) % HOME_HELP_TOPICS;
        else if(key == Key::down) topic_ = (topic_ + 1) % HOME_HELP_TOPICS;
        else if(key == Key::a) {
            if(home_topic_new_list(topic_)) { page_ = Page::new_list; return; }
            if(home_topic_flashcard_angle(topic_)) { page_ = Page::flashcard_angle; return; }
            if(home_topic_save_settings(topic_)) { page_ = Page::save_settings; return; }
            if(home_topic_hide_elements(topic_)) { page_ = Page::hide_elements; return; }
            help_page_ = topics[topic_].first;
            page_ = topics[topic_].credits ? Page::credits : Page::controls;
        }
        return;
    }
    if(page_ == Page::new_list) {
        if(key == Key::b) page_ = Page::help;
        else if(key == Key::a) {
            origin_ = Page::new_list; save_first_ = false;
            if(active && dirty) page_ = Page::confirm;
            else request_ = Request::create;
        }
    }
}
void HomeScreen::finish(bool success) {
    if(!success) { request_ = Request::none; page_ = origin_; save_first_ = false; }
}

HomeResult home_apply_request(const HomeScreen& h,HomeActions actions,const char* filename) {
    if(h.request()!=HomeScreen::Request::load && h.request()!=HomeScreen::Request::create)
        return HomeResult::open_failed;
    if(h.save_first() && !actions.save(actions.context)) return HomeResult::save_failed;
    bool ok=h.request()==HomeScreen::Request::create?
        actions.create(actions.context,filename):actions.load(actions.context,filename);
    return ok?HomeResult::opened:HomeResult::open_failed;
}

#ifndef HOME_SCREEN_HOST_TEST
#include "render.h"
#include "vocab_file_io.h"
#include "entries_screen.h"
#include "dictionary_fold.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include <cstring>
namespace {
struct HomePainter {
    Canvas& c;
    void ui(int x,int y,const char* s) { c.ui(x,y,s); }
    void ui_center(int y,const char* s) { c.ui_center(y,s); }
    void ui_grey(int x,int y,const char* s,bool right=false) { c.ui_grey(x,y,s,right); }
    void body(int x,int y,const char* s,int width=224) { c.body(x,y,s,width); }
};
bool any_home_key();
// Secret Settings and Save settings (gbamp3 style): the switches first, a
// line under them, then what they do (gbamp3 keeps switches and explanations
// apart the same way). Up/Down choose, A changes the value, B leaves.
const char* letter_box_label(LetterBox mode) {
    return mode == LetterBox::on ? "Letter Box: On" : mode == LetterBox::l_only ? "Letter Box: Only L+DPAD" : "Letter Box: Off";
}
enum class Setting { letter_box, save_after_changes, save_on_menu, hide_mode, hide_word_count, hide_boxes };
bool hide_setting(Setting s) { return s == Setting::hide_mode || s == Setting::hide_word_count || s == Setting::hide_boxes; }
HiddenElement hidden_element(Setting s) {
    return s == Setting::hide_mode ? HiddenElement::mode : s == Setting::hide_word_count ? HiddenElement::word_count : HiddenElement::boxes;
}
const char* setting_label(Setting s) {
    if(s == Setting::letter_box) return letter_box_label(entries_letter_box());
    if(hide_setting(s)) {
        static const char* const labels[HIDDEN_ELEMENTS][2] = {
            {"Hide Mode: Off", "Hide Mode: On"},
            {"Hide Total Word count: Off", "Hide Total Word count: On"},
            {"Hide Box indicator: Off", "Hide Box indicator: On"}};
        const HiddenElement e = hidden_element(s);
        return labels[int(e)][vocab_file_hidden(e) ? 1 : 0];
    }
    if(s == Setting::save_after_changes)
        return vocab_file_save_after_changes() ? "Save after entry changes: On" : "Save after entry changes: Off";
    return vocab_file_save_on_menu() ? "Save list on menu: On" : "Save list on menu: Off";
}
bool step_setting(Setting s) {
    if(s == Setting::letter_box) {
        LetterBox next = LetterBox((int(entries_letter_box()) + 1) % 3);
        const bool ok = vocab_file_set_letter_box(int(next));
        if(ok) entries_set_letter_box(next);
        return ok;
    }
    if(s == Setting::save_after_changes) return vocab_file_set_save_after_changes(!vocab_file_save_after_changes());
    if(hide_setting(s)) return vocab_file_set_hidden(hidden_element(s), !vocab_file_hidden(hidden_element(s)));
    return vocab_file_set_save_on_menu(!vocab_file_save_on_menu());
}
void run_settings(Canvas& c, const char* title, const Setting* rows, int count,
                  const char* (*line)(int), int lines) {
    int selected = 0;
    bool redraw = true;
    while(any_home_key()) bn::core::update();
    for(;;) {
        if(redraw) {
            const char* items[3];
            for(int i = 0; i < count; ++i) items[i] = setting_label(rows[i]);
            entries_draw_menu(c, title, items, count, selected, nullptr);
            c.fill(0, entries::ROW_Y + count * entries::ROW_H, 240, 1, ui_colors::GREY);
            for(int i = 0; i < lines; ++i)
                c.body(8, entries::ROW_Y + (i + count) * entries::ROW_H + 1, line(i));
            c.flip();
            redraw = false;
        } else bn::core::update();
        if(bn::keypad::b_pressed()) return;
        if(bn::keypad::up_pressed()) { selected = (selected + count - 1) % count; redraw = true; continue; }
        if(bn::keypad::down_pressed()) { selected = (selected + 1) % count; redraw = true; continue; }
        if(!bn::keypad::a_pressed()) continue;
        entries_show_saving(c);   // GBAVOCAB.SYS is written
        if(!step_setting(rows[selected])) {
            const char* failed[] = {"Not saved:", vocab_file_last_error()};
            run_notice(c, title, failed, 2);
        }
        while(any_home_key()) bn::core::update();
        redraw = true;
    }
}
void run_secret_settings(Canvas& c) {
    static const Setting rows[] = {Setting::letter_box};
    run_settings(c, "Secret Settings", rows, 1, home_secret_line, HOME_SECRET_LINES);
}
void run_save_settings(Canvas& c) {
    static const Setting rows[] = {Setting::save_after_changes, Setting::save_on_menu};
    run_settings(c, "Save settings", rows, 2, home_save_line, HOME_SAVE_LINES);
}
// Settings > Hide elements: three switches, all Off by default.
void run_hide_elements(Canvas& c) {
    static const Setting rows[] = {Setting::hide_mode, Setting::hide_word_count, Setting::hide_boxes};
    run_settings(c, "Hide elements", rows, 3, home_hide_line, HOME_HIDE_LINES);
}
// Settings > Flashcard angle, like gbareader's Angle screen: the four angles,
// the chosen one ticked; Up/Down move, A chooses (saved at once in
// GBAVOCAB.SYS), B leaves. A line, then what it does.
void run_flashcard_angle(Canvas& c) {
    const char* title = "Flashcard angle";
    int selected = int(vocab_file_flashcard_angle());
    bool redraw = true;
    while(any_home_key()) bn::core::update();
    for(;;) {
        if(redraw) {
            const char* items[FLASHCARD_ANGLES];
            for(int i = 0; i < FLASHCARD_ANGLES; ++i) items[i] = flashcard_angle_name(FlashcardAngle(i));
            entries_draw_menu(c, title, items, FLASHCARD_ANGLES, selected, nullptr);
            const int chosen = int(vocab_file_flashcard_angle());
            entries_draw_tick(c, entries::ROW_Y + chosen * entries::ROW_H, items[chosen]);
            c.fill(0, entries::ROW_Y + FLASHCARD_ANGLES * entries::ROW_H, 240, 1, ui_colors::GREY);
            for(int i = 0; i < HOME_ANGLE_LINES; ++i)
                c.body(8, entries::ROW_Y + (i + FLASHCARD_ANGLES) * entries::ROW_H + 1, home_angle_line(i));
            c.flip();
            redraw = false;
        } else bn::core::update();
        if(bn::keypad::b_pressed()) return;
        if(bn::keypad::up_pressed()) { selected = (selected + FLASHCARD_ANGLES - 1) % FLASHCARD_ANGLES; redraw = true; continue; }
        if(bn::keypad::down_pressed()) { selected = (selected + 1) % FLASHCARD_ANGLES; redraw = true; continue; }
        if(!bn::keypad::a_pressed() || FlashcardAngle(selected) == vocab_file_flashcard_angle()) continue;
        entries_show_saving(c);   // GBAVOCAB.SYS is written
        if(!vocab_file_set_flashcard_angle(FlashcardAngle(selected))) {
            const char* failed[] = {"Not saved:", vocab_file_last_error()};
            run_notice(c, title, failed, 2);
        }
        while(any_home_key()) bn::core::update();
        redraw = true;
    }
}
// The Settings list (Select in the main menu) as a gbamp3 list: New List,
// Flashcard angle, Save settings and Hide elements first, Credits last.
class TopicList final : public EntriesSource {
public:
    uint32_t count() override { return HOME_HELP_TOPICS; }
    bool text(uint32_t row, char out[192]) override {
        std::strcpy(out, home_topic_name(int(row)));
        if(home_topic_flashcard_angle(int(row))) {
            std::strcat(out, ": ");
            std::strcat(out, flashcard_angle_name(vocab_file_flashcard_angle()));
        }
        return true;
    }
    // New List, Flashcard angle, Save settings and Hide elements are
    // actions; a line separates them from the help topics.
    bool underline(uint32_t row) override { return row == 3; }
};
bool any_home_key() {
    return bn::keypad::a_held() || bn::keypad::b_held() || bn::keypad::start_held() ||
        bn::keypad::select_held() || bn::keypad::up_held() || bn::keypad::down_held() ||
        bn::keypad::left_held() || bn::keypad::right_held() || bn::keypad::l_held() || bn::keypad::r_held();
}
bool list_dirty(const VocabFile& vf) { return vocab_any_dirty(vf) || vf.array_generation || vf.pair_dirty; }

// The main menu: "Dictionaries" (book icon, line under it), then every list
// A to Z without ".txt"; the open list has the play mark. Names are copied
// into the idle work RAM and sorted with the dictionary fold (case and Latin
// accents ignored), so L + Up/Down steps by first letter.
class MainMenu final : public EntriesSource {
public:
    MainMenu(bool destination, const VocabFile& vf) : destination_(destination), vf_(vf) {}
    void build() {
        unsigned size = 0;
        unsigned char* ram = vocab_file_work_ram(size);
        offsets_ = reinterpret_cast<uint32_t*>(ram);
        order_ = reinterpret_cast<uint16_t*>(ram + MAX * 4);
        names_ = reinterpret_cast<char*>(ram + MAX * 6);
        const unsigned cap = size > MAX * 6 ? size - MAX * 6 : 0;
        n_ = 0;
        unsigned used = 0;
        const int total = vocab_file_count();
        for(int i = 0; i < total && n_ < MAX; ++i) {
            const char* name = vocab_file_name(i);
            if(!name) continue;
            const unsigned length = unsigned(std::strlen(name)) + 1;
            if(used + length > cap) break;
            std::memcpy(names_ + used, name, length);
            offsets_[n_] = used; order_[n_] = uint16_t(n_); ++n_; used += length;
        }
        // Shell sort of the order (at most MAX names).
        for(unsigned gap = n_ / 2; gap; gap /= 2)
            for(unsigned i = gap; i < n_; ++i)
                for(unsigned j = i; j >= gap && before(order_[j], order_[j - gap]); j -= gap) {
                    uint16_t t = order_[j]; order_[j] = order_[j - gap]; order_[j - gap] = t;
                }
    }
    uint32_t count() override { return lead() + (n_ ? n_ : 1); }
    bool divider(uint32_t row) override { return !n_ && row == lead(); }
    bool text(uint32_t row, char out[192]) override {
        if(is_dictionaries(row)) { std::strcpy(out, "Dictionaries"); return true; }
        if(!n_) { std::strcpy(out, vocab_file_sd_ready() ? "No lists in /gbavocab" : "No SD card"); return true; }
        const char* name = list_name(row);
        unsigned n = unsigned(std::strlen(name));
        if(n > 4 && name[n - 4] == '.' && (name[n - 3] | 32) == 't' && (name[n - 2] | 32) == 'x' && (name[n - 1] | 32) == 't') n -= 4;
        std::memcpy(out, name, n); out[n] = 0;
        return true;
    }
    int icon(uint32_t row) override {
        if(is_dictionaries(row)) return ICON_BOOK;
        const char* name = list_name(row);
        return name && vf_.loaded && !std::strcmp(name, vocab_file_loaded_name()) ? ICON_PLAY : ICON_NONE;
    }
    bool underline(uint32_t row) override { return is_dictionaries(row); }
    bool alphabetical() override { return n_ > 1; }
    bool letter(uint32_t row, uint32_t& key, char label[8]) override {
        const char* name = list_name(row);
        if(!name) { key = 0; label[0] = 0; return true; }    // Dictionaries stays first
        return entries_letter_of(name, key, label);
    }
    bool is_dictionaries(uint32_t row) const { return !destination_ && row == 0; }
    const char* list_name(uint32_t row) const {
        return n_ && row >= lead() && row - lead() < n_ ? names_ + offsets_[order_[row - lead()]] : nullptr;
    }
    bool row_of(const char* name, uint32_t& row) const {
        for(unsigned i = 0; i < n_; ++i) if(!std::strcmp(names_ + offsets_[order_[i]], name)) { row = lead() + i; return true; }
        return false;
    }
    static bool skip(void* self, uint32_t row) { return static_cast<MainMenu*>(self)->divider(row); }
private:
    static constexpr unsigned MAX = 2000;
    uint32_t lead() const { return destination_ ? 0 : 1; }
    bool before(uint16_t a, uint16_t b) const {
        const char* x = names_ + offsets_[a]; const char* y = names_ + offsets_[b];
        int c = dictionary_fold::compare(x, y);
        return c ? c < 0 : std::strcmp(x, y) < 0;
    }
    bool destination_;
    const VocabFile& vf_;
    uint32_t* offsets_ = nullptr; uint16_t* order_ = nullptr; char* names_ = nullptr; unsigned n_ = 0;
};

void home_draw(HomePainter& p,const HomeScreen& h,const char* name) {
    (void)name;
    using Page = HomeScreen::Page;
    if(h.page()==Page::controls || h.page()==Page::credits) {
        // gbareader About pages: centred title, grey page number, one line per
        // list row, "#" lines as grey subheadings, no instruction line.
        bool controls=h.page()==Page::controls;
        int page=h.help_page();
        p.ui_center(0,controls?home_help_heading(page):home_credit_heading(page));
        char number[8];int pages=home_topic_pages(h.topic()),at=page-home_topic_first(h.topic())+1,n=0;
        if(at>=10)number[n++]=char('0'+at/10);
        number[n++]=char('0'+at%10);number[n++]='/';
        if(pages>=10)number[n++]=char('0'+pages/10);
        number[n++]=char('0'+pages%10);number[n]=0;
        p.ui_grey(232,0,number,true);
        for(int i=0;i<HOME_PAGE_LINES;++i) {
            const char* line=controls?home_help_line(page,i):home_credit_line(page,i);
            const int y=entries::ROW_Y+i*entries::ROW_H;
            if(line[0]=='#') p.ui_grey(8,y+1,line+1);
            else if(line[0]) p.body(8,y+1,line);
        }
        return;
    }
}

// Settings, Controls, Credits, New List, Flashcard angle, Save settings, Hide elements and the unsaved-list question, until the
// main menu is shown again or a request is made.
void run_pages(Canvas& c,HomeScreen& h,const VocabFile& vf,char* new_name) {
    HomePainter p{c};
    bool wait_release=true,redraw=true;
    while(h.page()!=HomeScreen::Page::main && h.request()==HomeScreen::Request::none) {
        if(h.page()==HomeScreen::Page::secret) {
            run_secret_settings(c);
            h.press(HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            wait_release=redraw=true;
            continue;
        }
        if(h.page()==HomeScreen::Page::flashcard_angle) {
            run_flashcard_angle(c);
            h.press(HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            wait_release=redraw=true;
            continue;
        }
        if(h.page()==HomeScreen::Page::hide_elements) {
            run_hide_elements(c);
            h.press(HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            wait_release=redraw=true;
            continue;
        }
        if(h.page()==HomeScreen::Page::save_settings) {
            run_save_settings(c);
            h.press(HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            wait_release=redraw=true;
            continue;
        }
        if(h.page()==HomeScreen::Page::confirm) {
            // gbamp3 question: the choices are rows; B cancels.
            const char* lines[]={"Save changes before switching?","Failed switch keeps old list.",""};
            static const char* const items[]={"Save","Discard","Cancel"};
            const int r=run_choice(c,"Unsaved list",lines,3,items,3);
            h.press(r==0?HomeScreen::Key::a:r==1?HomeScreen::Key::b:HomeScreen::Key::select,vf.loaded,list_dirty(vf));
            while(any_home_key()) bn::core::update();
            continue;
        }
        if(h.page()==HomeScreen::Page::new_list) {
            if(new_name[0]) {
                const char* lines[]={"#Create in /gbavocab",new_name,"Then Start+Select: add entries.",""};
                static const char* const items[]={"Create","Back"};
                const int r=run_choice(c,"New List",lines,4,items,2);
                h.press(r==0?HomeScreen::Key::a:HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            } else {
                const char* lines[]={"Cannot choose an unused name.","Check the SD card."};
                run_notice(c,"New List",lines,2);
                h.press(HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            }
            while(any_home_key()) bn::core::update();
            continue;
        }
        if(h.page()==HomeScreen::Page::help) {
            TopicList topics;
            entries::Nav nav(HOME_HELP_TOPICS);nav.select(uint32_t(h.topic()));
            const bool open=run_entries(c,"Settings",topics,nav,entries_row_cache())==EntriesResult::open;
            if(open) h.choose_topic(int(nav.sel()));
            h.press(open?HomeScreen::Key::a:HomeScreen::Key::b,vf.loaded,list_dirty(vf));
            if(h.page()==HomeScreen::Page::new_list) {
                new_name[0]=0;
                if(!vocab_file_next_unused_name(new_name)) new_name[0]=0;
            }
            wait_release=redraw=true;
            continue;
        }
        HomeScreen::Page before=h.page();
        int old_help=h.help_page(),old_topic=h.topic();
        if(wait_release) { if(!any_home_key()) wait_release=false; }
        else {
            using K=HomeScreen::Key;
            const bool pressed[]={bn::keypad::select_pressed(),bn::keypad::start_pressed(),bn::keypad::b_pressed(),bn::keypad::a_pressed(),bn::keypad::up_pressed(),bn::keypad::down_pressed(),bn::keypad::left_pressed(),bn::keypad::right_pressed()};
            const K keys[]={K::select,K::start,K::b,K::a,K::up,K::down,K::left,K::right};
            for(int i=0;i<8;++i) if(pressed[i]) {
                if(!(h.page()==HomeScreen::Page::new_list && keys[i]==K::a && !new_name[0]))
                    h.press(keys[i],vf.loaded,list_dirty(vf));
                break;
            }
        }
        if(h.page()!=before) {
            wait_release=true;
            if(h.page()==HomeScreen::Page::new_list && before!=HomeScreen::Page::confirm) {
                new_name[0]=0;
                if(!vocab_file_next_unused_name(new_name)) new_name[0]=0;
            }
        }
        if(h.page()!=before || h.help_page()!=old_help || h.topic()!=old_topic) redraw=true;
        if(redraw && h.page()!=HomeScreen::Page::main && h.request()==HomeScreen::Request::none) {
            c.clear();home_draw(p,h,new_name);c.flip();redraw=false;
        } else bn::core::update();
    }
}
}

bool run_home_screen(Renderer& renderer,VocabFile& vf,HomeActions actions,bool* dictionary_requested,bool destination,bool* import_requested,const char* active_dictionary,bool* dictionary_resumed) {
    if(dictionary_requested)*dictionary_requested=false;
    if(dictionary_resumed)*dictionary_resumed=false;
    const bool dictionary_last=!destination && active_dictionary && active_dictionary[0] && dictionary_requested && dictionary_resumed;
    if(import_requested)*import_requested=false;
    renderer.reset();bn::core::update();bn::core::update();
    bool switched=false;
    {
        Canvas c;
        HomeScreen h(destination);
        // The cursor starts on the open list, else on the one opened last.
        char start[VOCAB_FILENAME_MAX]={};
        if(vf.loaded) std::strcpy(start,vocab_file_loaded_name());
        else { int last=vocab_file_last_list_index(); const char* n=last>=0?vocab_file_name(last):nullptr; if(n) std::strcpy(start,n); }
        MainMenu menu(destination,vf);
        menu.build();
        entries::Nav nav(menu.count(),MainMenu::skip,&menu);
        uint32_t row=0;
        if(start[0] && menu.row_of(start,row)) nav.select(row);
        char selected[VOCAB_FILENAME_MAX]={},new_name[VOCAB_FILENAME_MAX]={};
        bool done=false;
        while(!done) {
            if(h.page()==HomeScreen::Page::main) {
                EntriesOptions options;
                if(!destination) { options.exit_keys=entries::SELECT|entries::START; options.sleep=true; }   // L+R: sleep
                options.footer=dictionary_last?"B: Resume active dictionary":vf.loaded && !destination?"B: Resume active list":nullptr;
                auto result=run_entries(c,destination?"Add to which list?":"gbavocab",menu,nav,entries_row_cache(),&options);
                if(result==EntriesResult::back && dictionary_last) { *dictionary_requested=true; *dictionary_resumed=true; done=true; continue; }
                if(result==EntriesResult::back) h.back(vf.loaded);
                else if(result==EntriesResult::select) h.open_help();
                else if(result==EntriesResult::start) h.open_import();
                else if(menu.is_dictionaries(nav.sel())) h.open_dictionaries();
                else if(const char* name=menu.list_name(nav.sel())) {
                    std::strcpy(selected,name);
                    const bool is_open=vf.loaded && !std::strcmp(selected,vocab_file_loaded_name());
                    // As a destination the open list is simply chosen.
                    if(destination && is_open) { switched=true; done=true; continue; }
                    h.open_list(vf.loaded,list_dirty(vf),is_open);
                }
            } else {
                run_pages(c,h,vf,new_name);
                // Secret Settings, Save settings and Hide elements write GBAVOCAB.SYS through the
                // work RAM that holds the menu's names: rebuild them.
                menu.build();nav.resize(menu.count());
            }
            auto request=h.request();
            if(request==HomeScreen::Request::resume) done=true;
            else if(request==HomeScreen::Request::dictionary) { if(dictionary_requested)*dictionary_requested=true; done=true; }
            else if(request==HomeScreen::Request::import) { if(import_requested)*import_requested=true; done=true; }
            else if(request==HomeScreen::Request::load || request==HomeScreen::Request::create) {
                const char* target=request==HomeScreen::Request::create?new_name:selected;
                // Saving first: "save..." in front of the current screen until
                // the save (and the opening that follows) is done.
                if(h.save_first()) entries_show_saving(c);
                else { const char* lines[]={"Opening list..."}; draw_message(c,"gbavocab",lines,1); }
                HomeResult result=home_apply_request(h,actions,target);
                if(result==HomeResult::opened) { switched=true; done=true; }
                else {
                    const char* failed[]={result==HomeResult::save_failed?"SAVE FAILED - not switched":"OPEN FAILED - active list kept"};
                    run_notice(c,"Not opened",failed,1);
                    while(any_home_key()) bn::core::update();
                    h.finish(false);
                    // Saving and loading reuse the work RAM holding the names.
                    menu.build();nav.resize(menu.count());
                }
            }
        }
        // Do not leak the opening A or resume B into learning judgments.
        while(any_home_key()) bn::core::update();
    }
    bn::core::update();bn::core::update();renderer.reset();
    return switched;
}
#endif
