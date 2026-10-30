#include "import_screen.h"
#include "import_files.h"
#include "dict_build.h"
#include "entries_screen.h"
#include "entry_editor.h"
#include "entry_render.h"
#include "entry_screen.h"
#include "dictionary_handle.h"
#include "vocab_file_io.h"
#include "render.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include <cstring>

namespace {
uint16_t keys() {
    const bool held[] = {bn::keypad::up_held(), bn::keypad::down_held(), bn::keypad::left_held(), bn::keypad::right_held(),
        bn::keypad::a_held(), bn::keypad::b_held(), bn::keypad::l_held(), bn::keypad::r_held(),
        bn::keypad::start_held(), bn::keypad::select_held()};
    uint16_t v = 0;
    for (int i = 0; i < 10; ++i) if (held[i]) v |= uint16_t(1u << i);
    return v;
}
void release() { while (keys()) bn::core::update(); }

void notice(Canvas& c, const char* title, const char* a, const char* b = nullptr, const char* d = nullptr) {
    const char* lines[] = {a, b, d};
    run_notice(c, title, lines, 3);
    release();
}

class FolderSource final : public EntriesSource {
public:
    explicit FolderSource(ImportFolder& folder) : folder_(folder) {}
    uint32_t count() override { return uint32_t(folder_.count()); }
    bool text(uint32_t row, char out[192]) override {
        const char* name = folder_.name(int(row));
        if (!name) return false;
        std::strcpy(out, name);
        if (folder_.is_folder(int(row))) std::strcat(out, "/");
        return true;
    }
private:
    ImportFolder& folder_;
};

void title_for(const char* folder, char out[40]) {
    if (!folder[1]) { std::strcpy(out, "Import to /gbavocab"); return; }
    unsigned n = unsigned(std::strlen(folder));
    if (n < 36) { std::strcpy(out, folder); return; }
    const char* tail = folder + n - 32;
    while ((static_cast<unsigned char>(*tail) & 0xc0) == 0x80) ++tail;
    std::strcpy(out, "...");
    std::strcat(out, tail);
}

// A name in /gbavocab for the copy: asks before using a numbered name.
bool ask_target(Canvas& c, const char* name, unsigned limit, char (&target)[IMPORT_NAME_MAX]) {
    switch (import_target_name(name, limit, target)) {
    case ImportName::free: return true;
    case ImportName::numbered: {
        char line[80]; std::strcpy(line, name); std::strcat(line, " exists.");
        char as[80]; std::strcpy(as, target); std::strcat(as, " ?");
        const char* lines[] = {line, "Nothing is replaced. Import as", as, ""};
        static const char* const no_yes[] = {"No", "Yes"};
        bool yes = run_choice(c, "Name already used", lines, 4, no_yes, 2) == 1;
        release();
        return yes;
    }
    case ImportName::too_long: {
        char line[40] = "Name too long: max ";
        import_number(line + std::strlen(line), limit);
        std::strcat(line, " bytes.");
        notice(c, "Not imported", line, "Rename it on a PC.");
        return false;
    }
    default: notice(c, "Not imported", "No free name (2 to 99).", "Rename it on a PC."); return false;
    }
}

struct CopyView { Canvas* c; const char* name; };
void copy_progress(void* context, int percent) {
    auto& v = *static_cast<CopyView*>(context);
    char line[16]; import_number(line, unsigned(percent)); std::strcat(line, " %");
    const char* lines[] = {v.name, line, "", "Do not switch off."};
    draw_message(*v.c, "Importing", lines, 4);
}

void copy_into_library(Canvas& c, const char* source, const char* name, bool list) {
    char target[IMPORT_NAME_MAX];
    if (!ask_target(c, name, list ? IMPORT_LIST_NAME_MAX : IMPORT_NAME_MAX - 1, target)) return;
    char path[IMPORT_PATH_MAX];
    import_join(path, "/gbavocab", target);
    unsigned size = 0;
    unsigned char* buffer = vocab_file_work_ram(size);
    CopyView view{&c, target};
    ImportCopy r = import_copy(source, path, buffer, size > 16384 ? 16384 : size, copy_progress, &view);
    if (r == ImportCopy::copied) notice(c, "Imported", list ? "Imported as list:" : "Imported dictionary:", target, "in /gbavocab");
    else notice(c, "Not imported", r == ImportCopy::read_failed ? "Cannot read the file." : "Cannot write (card full?)", "Nothing was changed.");
}

// Language codes with the typing controls: Start+A next / build, Start+B back.
const EntryEditor* s_codes_editor = nullptr;
bool s_codes_for_list = false;
void codes_line(void* context, int x, int y, const char* text) {
    auto& c = *static_cast<Canvas*>(context);
    const bool back = s_codes_editor->screen() == EntryEditor::Screen::back;
    if (y == 0) text = back ? "Language codes 2/2" : "Language codes 1/2";
    if (y == 18) text = back ? "Second language code (e.g. en)" : "First language code (e.g. de)";
    if (y == 144) text = back ? (s_codes_for_list ? "Start+A: Save   Start+B: Back" : "Start+A: Build  Start+B: Back")
                              : "Start+A: Next   Start+B: Back";
    if (x < 0) c.ui_center(y, text); else c.ui(x, y, text);
}
}
// Two language codes typed with the entry editor (lowercased, checked).
bool ask_language_codes(Canvas& c, char first[12], char second_out[12], bool for_list) {
    DictBuildMeta meta;
    s_codes_for_list = for_list;
    auto& e = entry_draft_editor();
    EntryEditorLoan loan(e);
    e.open(-1, nullptr);
    e.set_languages(PairMetadata(), false, true);   // plain order, no list codes
    if (!e.prefill_add("", "")) return false;
    s_codes_editor = &e;
    while (e.active() && e.screen() != EntryEditor::Screen::menu) {
        e.frame(keys());
        if (e.commit_requested()) {
            char row[VOCAB_RAW_LINE_MAX]; std::strcpy(row, e.row());
            char* tab = std::strchr(row, '\t');
            *tab = 0;
            char* second = tab + 1;
            char* extra = std::strchr(second, '\t'); if (extra) *extra = 0;
            for (char* p = row; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = char(*p + 32);
            for (char* p = second; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = char(*p + 32);
            if (PairMetadata::code(row) && PairMetadata::code(second) && std::strcmp(row, second)) {
                std::strcpy(meta.front, row); std::strcpy(meta.back, second);
                dict_build_label(meta.front, meta.front_label); dict_build_label(meta.back, meta.back_label);
                std::strcpy(first, meta.front); std::strcpy(second_out, meta.back);
                e.finish(true, "");
                release();
                return true;
            }
            e.finish(false, "a-z 0-9 -, max 11, different");
        }
        c.clear();
        if (e.active()) render_entry(e, c.pixels, codes_line, &c);
        c.flip();
    }
    release();
    return false;
}
namespace {
bool ask_codes(Canvas& c, DictBuildMeta& meta) {
    char first[12], second[12];
    if (!ask_language_codes(c, first, second, false)) return false;
    std::strcpy(meta.front, first); std::strcpy(meta.back, second);
    dict_build_label(meta.front, meta.front_label); dict_build_label(meta.back, meta.back_label);
    return true;
}

struct BuildView { Canvas* c; const char* name; };
void build_progress(void* context, int step, int percent) {
    static const char* const steps[] = {"", "1/4 Reading the file", "2/4 Sorting first language", "3/4 Sorting second language", "4/4 Writing the dictionary"};
    auto& v = *static_cast<BuildView*>(context);
    char line[16]; import_number(line, unsigned(percent)); std::strcat(line, " %");
    const char* lines[] = {v.name, "", steps[step < 1 || step > 4 ? 0 : step], line, "", "Do not switch off."};
    draw_message(*v.c, "Building dictionary", lines, 6);
}

void build_dictionary(Canvas& c, const char* source, const char* name, bool in_library) {
    const char* lines[] = {"Create a dictionary from", name, "This can take some minutes.",
                           in_library ? "The list stays unchanged." : "The TXT stays unchanged.", ""};
    static const char* const go_back[] = {"Create dictionary", "Back"};
    bool go = run_choice(c, "Import as dictionary", lines, 5, go_back, 2) == 0;
    release();
    if (!go) return;
    DictBuildWorkspace w;
    w.ram = vocab_file_work_ram(w.ram_size);
    for (int i = 0; i < 3; ++i) w.io[i] = vocab_file_dictionary_window(i);
    DictBuildMeta meta;
    if (!dict_build_detect(source, meta, w) && !ask_codes(c, meta)) return;
    if (!meta.name[0]) dict_build_name_from_file(name, meta.name);
    char dict_name[IMPORT_NAME_MAX];
    unsigned n = unsigned(std::strlen(name));
    if (n > 4) n -= 4;
    if (n + 5 >= IMPORT_NAME_MAX) n = IMPORT_NAME_MAX - 6;
    std::memcpy(dict_name, name, n); std::strcpy(dict_name + n, ".dict");
    char target[IMPORT_NAME_MAX];
    if (!ask_target(c, dict_name, IMPORT_NAME_MAX - 1, target)) return;
    char path[IMPORT_PATH_MAX];
    import_join(path, "/gbavocab", target);
    BuildView view{&c, target};
    DictBuildStats stats;
    DictBuildError e = dict_build(source, path, meta, w, stats, build_progress, &view);
    if (e != DictBuildError::none) { notice(c, "Dictionary not created", dict_build_error_text(e), "Nothing was changed."); return; }
    char count[40]; import_number(count, stats.imported); std::strcat(count, " entries");
    char skipped[64] = "Skipped: ";
    import_number(skipped + std::strlen(skipped), stats.too_long); std::strcat(skipped, " too long,");
    char skipped2[64] = "";
    import_number(skipped2, stats.invalid); std::strcat(skipped2, " invalid, ");
    import_number(skipped2 + std::strlen(skipped2), stats.duplicates); std::strcat(skipped2, " duplicates");
    const char* result[] = {"Dictionary created:", target, count, "", skipped, skipped2};
    run_notice(c, "Imported", result, 6);
    release();
}

void import_list(Canvas& c, const char* source, const char* name) {
    unsigned size = 0;
    unsigned char* buffer = vocab_file_work_ram(size);
    bool ok = false;
    const char* lines[] = {name, "", "Counting entries..."};
    draw_message(c, "Import as list", lines, 3);
    uint32_t rows = import_count_rows(source, buffer, size, IMPORT_LIST_ROWS, ok);
    if (!ok) { notice(c, "Not imported", "Cannot read the file."); return; }
    if (rows > IMPORT_LIST_ROWS) {
        const char* big[] = {"Too big for a list:", "15,000 entries max.", "", "Import as dictionary?", ""};
        static const char* const no_yes[] = {"No", "Yes"};
        bool yes = run_choice(c, "Import as list", big, 5, no_yes, 2) == 1;
        release();
        if (yes) build_dictionary(c, source, name, false);
        return;
    }
    copy_into_library(c, source, name, true);
}
}

void import_number(char* out, uint32_t value) {
    char digits[16]; int n = 0;
    do { digits[n++] = char('0' + value % 10); value /= 10; } while (value);
    int j = 0;
    for (int i = n - 1; i >= 0; --i) { out[j++] = digits[i]; if (i && i % 3 == 0) out[j++] = ','; }
    out[j] = 0;
}

void run_import_screen(Renderer& renderer) {
    renderer.reset(); bn::core::update(); bn::core::update();
    {
        Canvas c;
        if (!vocab_file_sd_ready()) { notice(c, "Import", "No SD card / storage unavailable."); }
        else {
            char path[IMPORT_PATH_MAX] = "/";
            ImportFolder folder(reinterpret_cast<char*>(entries_order_scratch()));
            FolderSource source(folder);
            entries::Nav nav;
            bool opened = folder.open(path);
            nav.reset(source.count(), nullptr, nullptr);
            if (!opened) notice(c, "Import", "Cannot read the SD card.");
            while (opened) {
                char title[40]; title_for(path, title);
                if (run_entries(c, title, source, nav, entries_row_cache()) == EntriesResult::back) {
                    char leaf[IMPORT_NAME_MAX]; std::strncpy(leaf, import_leaf(path), IMPORT_NAME_MAX - 1); leaf[IMPORT_NAME_MAX - 1] = 0;
                    if (!import_parent(path)) break;
                    opened = folder.open(path);
                    nav.reset(source.count(), nullptr, nullptr);
                    // The folder just left stays selected.
                    for (int i = 0; i < folder.count() && folder.is_folder(i); ++i) {
                        const char* n = folder.name(i);
                        if (n && !std::strcmp(n, leaf)) { nav.select(uint32_t(i)); break; }
                    }
                    continue;
                }
                const int index = int(nav.sel());
                const char* entry = folder.name(index);
                if (!entry) continue;
                char name[IMPORT_NAME_MAX]; std::strcpy(name, entry);
                char full[IMPORT_PATH_MAX];
                if (!import_join(full, path, name)) { notice(c, "Import", "Path too long."); continue; }
                if (folder.is_folder(index)) {
                    std::strcpy(path, full);
                    opened = folder.open(path);
                    nav.reset(source.count(), nullptr, nullptr);
                    if (!opened) { notice(c, "Import", "Cannot read this folder."); import_parent(path); opened = folder.open(path); nav.reset(source.count(), nullptr, nullptr); }
                    continue;
                }
                const bool library = import_in_library(path);
                if (import_is_dict(name)) {
                    if (library) notice(c, "Import", name, "is already in /gbavocab.");
                    else copy_into_library(c, full, name, false);
                } else if (library) {
                    build_dictionary(c, full, name, true);
                } else {
                    static const char* const items[] = {"List", "Dictionary"};
                    int item = run_menu(c, "Import as", items, nullptr, 2);
                    release();
                    if (item == 0) import_list(c, full, name);
                    else if (item == 1) build_dictionary(c, full, name, false);
                }
                // The folder may have changed (files added to /gbavocab).
                uint32_t keep = nav.sel();
                opened = folder.open(path);
                nav.reset(source.count(), nullptr, nullptr);
                nav.select(keep);
            }
        }
        release();
    }
    vocab_file_rescan();
    bn::core::update(); bn::core::update(); renderer.reset();
}
