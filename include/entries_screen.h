#pragma once
#include "entries_nav.h"
#include "ui_canvas.h"
#include "save_status.h"

// Rows of a long list. Text is read only for rows that come into view.
class EntriesSource {
public:
    virtual uint32_t count() = 0;
    // Box dividers: shown, never selected.
    virtual bool divider(uint32_t) { return false; }
    // One line: the entry, or the divider label. False shows "?".
    virtual bool text(uint32_t row, char out[192]) = 0;
    // Alphabetical lists: L + Up/Down letter steps and the letter box.
    virtual bool alphabetical() { return false; }
    virtual bool letter(uint32_t, uint32_t&, char[8]) { return false; }
    // Start: show the other language first. sel is updated to the row now
    // holding the selected entry. False: no change.
    virtual bool swap(uint32_t&) { return false; }
    // Icon at the right end of a row (ICON_BOOK, ICON_PLAY) and a thin line
    // under a row (the main menu's Dictionaries row).
    virtual int icon(uint32_t) { return 0; }
    virtual bool underline(uint32_t) { return false; }
protected:
    ~EntriesSource() = default;
};

constexpr int ICON_NONE = 0, ICON_BOOK = 1, ICON_PLAY = 2;
enum class EntriesResult { back, open, select, start };
struct EntriesOptions {
    uint16_t exit_keys = 0;          // entries::SELECT / entries::START: return on that key
    const char* footer = nullptr;    // grey bottom line (7 rows instead of 8)
    // Main menu: L+R together (the moment both are down) puts the GBA to sleep
    // (entries_sleep); START held ENTRIES_WAKE_HOLD frames wakes it on the
    // same screen (a shorter START leaves it asleep). That START does
    // nothing else, and every button is ignored until all of them have been
    // up for ENTRIES_WAKE_QUIET frames (a bouncing START opens nothing).
    bool sleep = false;
};
constexpr unsigned ENTRIES_WAKE_QUIET = 6;
constexpr unsigned ENTRIES_WAKE_HOLD = 60;   // 1 s (gbamp3 Hold START to unlock)
// Sleep mode (gbamp3 lock + sleep): the screen off and the GBA in Stop mode
// until START has been held ENTRIES_WAKE_HOLD frames (released earlier: Stop
// again, the screen still off); returns with START still held.
void entries_sleep();
constexpr unsigned ENTRIES_CACHE_BYTES = entries::ROWS * 192;
// gbamp3-style list: header, 8 rows, light-blue cursor, marquee on the
// selected row. Returns on B (back) or A (open nav.sel()). nav keeps the
// position between calls. cache: ENTRIES_CACHE_BYTES of scratch.
EntriesResult run_entries(Canvas& canvas, const char* title, EntriesSource& source,
                          entries::Nav& nav, char* cache, const EntriesOptions* options = nullptr);
// Letter box (Secret Settings): Off (default), On, or only on L + Up/Down.
enum class LetterBox { off = 0, on = 1, l_only = 2 };
void entries_set_letter_box(LetterBox mode);
LetterBox entries_letter_box();
// Letter group of a text for L + Up/Down: its first folded character
// (digits and signs: "#").
bool entries_letter_of(const char* text, uint32_t& key, char label[8]);
// Small menu in the same style. confirm[i] (optional, may be null): the first
// A shows that question in the row; A again chooses, B keeps the menu.
// Returns the chosen row, or -1 on B.
int run_menu(Canvas& canvas, const char* title, const char* const* items,
             const char* const* confirm, int count, int selected = 0);
// The learning screen's "save..." at the top right, in front of whatever this
// canvas shows (the screen itself is not redrawn); it stays until the next
// redraw, so call it right before a save.
void entries_show_saving(Canvas& canvas);
// One frame of a gbamp3 menu (as run_menu draws it) for screens with their own
// input loop; footer: optional grey last line (a message).
void entries_draw_menu(Canvas& canvas, const char* title, const char* const* items, int count, int selected, const char* footer);
// gbareader's check mark after a menu row's text (the row at y, as drawn).
void entries_draw_tick(Canvas& canvas, int y, const char* text);
// gbareader-style pages: centred title, one line per list row ("#": grey
// subheading), no instruction line. draw_message only draws (progress);
// run_notice waits for A or B; run_choice shows the lines, then the items
// as rows with the light-blue bar, and returns the chosen item or -1 on B.
void draw_message(Canvas& canvas, const char* title, const char* const* lines, int count);
void run_notice(Canvas& canvas, const char* title, const char* const* lines, int count);
int run_choice(Canvas& canvas, const char* title, const char* const* lines, int line_count,
               const char* const* items, int item_count, int selected = 0);
// One frame of run_choice (no flip) for screens with their own input loop.
void entries_draw_choice(Canvas& canvas, const char* title, const char* const* lines, int line_count,
                         const char* const* items, int item_count, int selected);
// Divider text between boxes ("Box 1 above / Box 2 below").
void entries_box_label(int above, int below, char out[48]);
// Borrowed scratch (main.cpp: buffers idle on the GBA once a list is on SD).
char* entries_row_cache();                   // >= 2048 bytes
unsigned char* entries_order_scratch();      // >= 4096 bytes
