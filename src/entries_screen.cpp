// gbavocab V4.0 long lists (List entries, Dictionary list entries) and small
// menus, in the look and with the controls of gbamp3 v1.8's long lists.
#include "entries_screen.h"
#include "dictionary_fold.h"
#include <cstring>
#ifndef UI_CANVAS_HOST
#include "bn_core.h"
#include "bn_core_lock.h"
#include "bn_keypad.h"
#endif

using entries::Input;
using entries::Nav;
using entries::Steps;
using entries::ROWS;
using entries::ROW_Y;
using entries::ROW_H;

#ifndef UI_CANVAS_HOST
unsigned ui_text_width(const char* s) { return font_width(s); }
unsigned ui_text_fit(const char* s, unsigned width) { return font_width_cap(s, width); }
#endif

namespace {
LetterBox letter_box_mode = LetterBox::off;
}
void entries_set_letter_box(LetterBox mode) { letter_box_mode = mode; }
LetterBox entries_letter_box() { return letter_box_mode; }

namespace {
uint16_t list_keys() {
    const bool held[] = {bn::keypad::up_held(), bn::keypad::down_held(), bn::keypad::left_held(), bn::keypad::right_held(),
        bn::keypad::a_held(), bn::keypad::b_held(), bn::keypad::l_held(), bn::keypad::r_held(),
        bn::keypad::start_held(), bn::keypad::select_held()};
    uint16_t v = 0;
    for (int i = 0; i < 10; ++i) if (held[i]) v |= uint16_t(1u << i);
    return v;
}

constexpr int TEXT_X = 6, TEXT_RIGHT = 234;
constexpr int BOX_X = 99, BOX_Y = 55, BOX_W = 42, BOX_H = 41;
constexpr unsigned BOX_LINGER = 36, BOX_HOLD = 90, MARQUEE_WAIT = 60;

// gbamp3 marquee: wait, scroll one pixel every second frame to the end, hold, restart.
struct Marquee {
    unsigned off = 0, wait = MARQUEE_WAIT, hold = 0, frame = 0;
    void reset() { off = 0; wait = MARQUEE_WAIT; hold = 0; }
    // True when the selected row must be redrawn.
    bool step(unsigned width, unsigned avail) {
        ++frame;
        if (width <= avail) return false;
        if (wait) { --wait; return false; }
        if (width - avail <= off) {
            if (++hold < MARQUEE_WAIT) return false;
            reset();
            return true;
        }
        if (frame & 1) return false;
        ++off;
        return true;
    }
};

// gbamp3 icons, drawn at x 226 in the row's text colour.
void draw_icon(Canvas& c, int y, int icon, uint8_t bg) {
    if (icon == ICON_PLAY) {
        for (int j = 0; j < 9; ++j) c.fill(226, y + 4 + j, j < 5 ? j + 1 : 9 - j, 1, ui_colors::BLACK);
    } else if (icon == ICON_BOOK) {
        c.fill(226, y + 3, 10, 11, ui_colors::BLACK);    // cover
        c.fill(229, y + 4, 6, 9, bg);                    // pages, spine on the left
        c.fill(230, y + 6, 4, 1, ui_colors::BLACK);      // title
    }
}

void draw_row(Canvas& c, int y, const char* text, bool selected, unsigned marquee, int icon = ICON_NONE) {
    const uint8_t bg = selected ? ui_colors::BLUE : ui_colors::WHITE;
    if (selected) c.fill(0, y, 240, ROW_H, ui_colors::BLUE);
    draw_icon(c, y, icon, bg);
    const int right = icon ? 222 : TEXT_RIGHT;
    unsigned avail = unsigned(right - TEXT_X), width = ui_text_width(text);
    if (width <= avail) { c.body_from(TEXT_X, y + 1, text, 0, avail); return; }
    if (selected && marquee) { c.body_from(TEXT_X, y + 1, text, marquee, avail); return; }
    char shown[200];
    unsigned n = ui_text_fit(text, avail - ui_text_width("..."));
    if (n > sizeof(shown) - 4) n = sizeof(shown) - 4;
    std::memcpy(shown, text, n);
    std::memcpy(shown + n, "...", 4);
    c.body_from(TEXT_X, y + 1, shown, 0, avail);
}

void draw_divider(Canvas& c, int y, const char* label) {
    unsigned w = ui_text_width(label);
    int x = (240 - int(w)) / 2, mid = y + ROW_H / 2;
    if (x - 6 > TEXT_X) c.fill(TEXT_X, mid, x - 6 - TEXT_X, 1, ui_colors::GREY);
    if (x + int(w) + 6 < TEXT_RIGHT) c.fill(x + int(w) + 6, mid, TEXT_RIGHT - x - int(w) - 6, 1, ui_colors::GREY);
    c.body_from(x, y + 1, label, 0, 240 - x, ui_colors::GREY);
}

void draw_letter_box(Canvas& c, const char* label) {
    c.fill(BOX_X + 2, BOX_Y, BOX_W - 4, 1, ui_colors::BOX);
    c.fill(BOX_X + 1, BOX_Y + 1, BOX_W - 2, 1, ui_colors::BOX);
    c.fill(BOX_X, BOX_Y + 2, BOX_W, BOX_H - 4, ui_colors::BOX);
    c.fill(BOX_X + 1, BOX_Y + BOX_H - 2, BOX_W - 2, 1, ui_colors::BOX);
    c.fill(BOX_X + 2, BOX_Y + BOX_H - 1, BOX_W - 4, 1, ui_colors::BOX);
    unsigned w = ui_text_width(label);
    if (w > BOX_W / 2) w = BOX_W / 2;
    c.glyph2x(BOX_X + (BOX_W - 2 * int(w)) / 2, BOX_Y + (BOX_H - 32) / 2, label, ui_colors::WHITE);
}

// First row in [lo, hi) whose letter group is > key (after) or >= key.
bool group_bound(EntriesSource& s, uint32_t lo, uint32_t hi, uint32_t key, bool after, uint32_t& out) {
    while (lo < hi) {
        uint32_t m = lo + (hi - lo) / 2, k = 0;
        char label[8];
        if (!s.letter(m, k, label)) return false;
        if (after ? k <= key : k < key) lo = m + 1; else hi = m;
    }
    out = lo;
    return true;
}

// L + Up/Down: the first entry of the previous / next letter group. A fresh
// press past the first / last group wraps; holding stops there (gbamp3).
bool letter_step(EntriesSource& s, Nav& nav, int dir, bool fresh) {
    uint32_t n = nav.count(), k = 0, t = 0;
    char label[8];
    if (nav.empty() || !s.letter(nav.sel(), k, label)) return false;
    if (dir > 0) {
        if (!group_bound(s, nav.sel() + 1, n, k, true, t)) return false;
        if (t >= n) { if (!fresh) return false; t = 0; }
    } else {
        uint32_t start = 0, kp = 0;
        if (!group_bound(s, 0, nav.sel() + 1, k, false, start)) return false;
        if (!start) { if (!fresh || !s.letter(n - 1, kp, label)) return false; }
        else if (!s.letter(start - 1, kp, label)) return false;
        if (!group_bound(s, 0, start ? start : n, kp, false, t)) return false;
    }
    nav.select_on_top(t);
    return true;
}
}

bool entries_letter_of(const char* text, uint32_t& key, char label[8]) {
    dictionary_fold::Iterator it(text);
    unsigned c = it.next();
    if (c < 'a' || c >= 0x110000) { key = 1; label[0] = '#'; label[1] = 0; return true; }
    key = c;
    unsigned u = c <= 'z' ? c - 32 : c;
    if (u < 0x80) { label[0] = char(u); label[1] = 0; }
    else if (u < 0x800) { label[0] = char(0xc0 | (u >> 6)); label[1] = char(0x80 | (u & 63)); label[2] = 0; }
    else if (u < 0x10000) { label[0] = char(0xe0 | (u >> 12)); label[1] = char(0x80 | ((u >> 6) & 63)); label[2] = char(0x80 | (u & 63)); label[3] = 0; }
    else { label[0] = char(0xf0 | (u >> 18)); label[1] = char(0x80 | ((u >> 12) & 63)); label[2] = char(0x80 | ((u >> 6) & 63)); label[3] = char(0x80 | (u & 63)); label[4] = 0; }
    return true;
}

void entries_box_label(int above, int below, char out[48]) {
    char* p = out;
    auto put = [&](const char* s) { while (*s) *p++ = *s++; };
    auto digit = [&](int d) { *p++ = char('0' + d); };
    if (above) { put("Box "); digit(above); put(" above"); if (below) put("  /  "); }
    if (below) { put("Box "); digit(below); put(" below"); }
    *p = 0;
}

EntriesResult run_entries(Canvas& c, const char* title, EntriesSource& source, Nav& nav, char* cache,
                          const EntriesOptions* options)
{
    const uint16_t exits = options ? options->exit_keys : 0;
    const char* footer = options ? options->footer : nullptr;
    const int rows = footer ? ROWS - 1 : ROWS;
    nav.set_rows(rows);
    Input input;
    input.reset(list_keys());
    Marquee marquee;
    uint32_t cached_top = ~0u, last_sel = ~0u, last_top = ~0u;
    unsigned box_frames = 0, hold = 0;
    bool box_on = false, box_l = false, redraw = true;
    char box_label[8] = {};
    auto show_box = [&](unsigned frames) {
        uint32_t k = 0;
        if (letter_box_mode == LetterBox::off) return;
        char l[8];
        if (!source.alphabetical() || nav.empty() || !source.letter(nav.sel(), k, l) || !l[0]) return;
        std::memcpy(box_label, l, sizeof l);
        box_on = true;
        box_frames = frames;
        redraw = true;
    };
    const bool sleeps = options && options->sleep;
    bool waking = false;
    unsigned quiet = 0;
    for (;;) {
        uint16_t keys = list_keys();
        // After a wake-up the buttons do nothing until all are up for a moment.
        if (waking) {
            quiet = keys ? 0 : quiet + 1;
            if (quiet >= ENTRIES_WAKE_QUIET) waking = false;
            keys = 0;
        }
        Steps s = input.frame(keys);
        if (sleeps && (s.held & (entries::L | entries::R)) == (entries::L | entries::R) &&
            (s.pressed & (entries::L | entries::R))) {
            entries_sleep();
            waking = true;
            quiet = 0;
            input.reset(0);
            continue;
        }
        if (s.pressed & entries::B) return EntriesResult::back;
        if ((s.pressed & entries::A) && !nav.empty()) return EntriesResult::open;
        if ((exits & entries::SELECT) && (s.pressed & entries::SELECT)) return EntriesResult::select;
        if ((exits & entries::START) && (s.pressed & entries::START)) return EntriesResult::start;
        if ((s.pressed & entries::START) && s.held == entries::START) {
            uint32_t sel = nav.sel();
            if (source.swap(sel)) {
                nav.resize(source.count());
                nav.select(sel);
                cached_top = ~0u;
                marquee.reset();
                redraw = true;
            }
        }
        if (s.held & entries::L) {
            if (source.alphabetical())
                for (int j = 0; j < 2; ++j)
                    for (unsigned n = 0; n < s.letters[j]; ++n) {
                        bool fresh = !n && (s.pressed & (j ? entries::DOWN : entries::UP));
                        if (!letter_step(source, nav, j ? 1 : -1, fresh)) break;
                        show_box(0);
                        box_l = true;
                    }
        } else {
            if (box_l) { box_l = false; box_on = false; redraw = true; }   // L released: the box goes
            bool moved = false, wrapped = false;
            for (int j = 0; j < 2; ++j)
                for (unsigned n = 0; n < s.rows[j]; ++n) {
                    bool fresh = !n && (s.pressed & (j ? entries::DOWN : entries::UP));
                    uint32_t before = nav.sel();
                    if (nav.step(j ? 1 : -1, fresh)) {
                        moved = true;
                        if (fresh && (j ? nav.sel() < before : nav.sel() > before)) wrapped = true;
                    }
                }
            bool paged = false;
            for (int j = 0; j < 2; ++j)
                for (unsigned n = 0; n < s.pages[j]; ++n) if (nav.page(j ? 1 : -1)) paged = true;
            // The letter box shows while the list moves fast (gbamp3 v1.6).
            hold = (s.held & (entries::UP | entries::DOWN)) ? hold + 1 : 0;
            bool held_long = hold >= BOX_HOLD;
            // Only L + Up/Down: fast moves, pages and wraps show no box.
            if ((wrapped || paged || (held_long && moved)) && letter_box_mode == LetterBox::on) show_box(BOX_LINGER);
            else if (moved && box_on && !box_frames) { box_on = false; redraw = true; }
            else if (box_on && box_frames && !--box_frames) { box_on = false; redraw = true; }
        }
        if (nav.sel() != last_sel || nav.top() != last_top) {
            last_sel = nav.sel();
            last_top = nav.top();
            marquee.reset();
            redraw = true;
        }
        // Row texts are read only when the page changes.
        if (cached_top != nav.top()) {
            for (int r = 0; r < rows; ++r) {
                uint32_t i = nav.top() + uint32_t(r);
                char* t = cache + r * 192;
                if (i >= nav.count()) t[0] = 0;
                else if (!source.text(i, t)) std::strcpy(t, "?");
            }
            cached_top = nav.top();
            redraw = true;
        }
        if (!nav.empty() && !box_on) {
            const char* t = cache + (nav.sel() - nav.top()) * 192;
            const int right = source.icon(nav.sel()) ? 222 : TEXT_RIGHT;
            if (marquee.step(ui_text_width(t), unsigned(right - TEXT_X))) redraw = true;
        }
        if (!redraw) { bn::core::update(); continue; }
        redraw = false;
        c.clear();
        c.ui_center(0, title);
        if (!nav.count()) c.body((240 - int(ui_text_width("No entries"))) / 2, 72, "No entries");
        for (int r = 0; r < rows; ++r) {
            uint32_t i = nav.top() + uint32_t(r);
            if (i >= nav.count()) break;
            int y = ROW_Y + r * ROW_H;
            const char* t = cache + r * 192;
            if (source.divider(i)) draw_divider(c, y, t);
            else draw_row(c, y, t, i == nav.sel(), marquee.off, source.icon(i));
            if (source.underline(i)) c.fill(0, y + ROW_H - 1, 240, 1, ui_colors::GREY);
        }
        if (footer) c.body_from(TEXT_X, ROW_Y + ROWS * ROW_H - ROW_H + 1, footer, 0, TEXT_RIGHT - TEXT_X, ui_colors::GREY);
        if (box_on) draw_letter_box(c, box_label);
        c.flip();
    }
}

#ifndef UI_CANVAS_HOST
namespace {
volatile uint16_t& reg16(uintptr_t a) { return *reinterpret_cast<volatile uint16_t*>(a); }
constexpr uintptr_t DISPCNT = 0x04000000, VCOUNT = 0x04000006, KEYINPUT = 0x04000130, KEYCNT = 0x04000132,
                    IE = 0x04000200, IF = 0x04000202, IME = 0x04000208;
constexpr uint16_t START_BIT = 8, IRQ_KEYPAD = 0x1000, DISPCNT_BLANK = 0x80;
bool start_down() { return !(reg16(KEYINPUT) & START_BIT); }
void keypad_irq(bool on)
{
    uint16_t ime = reg16(IME);
    reg16(IME) = 0;
    // A keypad request left from the last START (raised while its interrupt
    // was off) would end the next Stop at once: cleared first.
    if (on) reg16(IF) = IRQ_KEYPAD;
    reg16(IE) = on ? uint16_t(reg16(IE) | IRQ_KEYPAD) : uint16_t(reg16(IE) & ~IRQ_KEYPAD);
    reg16(IME) = ime;
}
}

void entries_sleep()
{
    // gbamp3 lock + sleep with Hold START to unlock: the display off, a
    // keypad interrupt on START and BIOS Stop (bn::core::sleep's steps, with
    // Butano's interrupts off under a core_lock). After the interrupt START
    // must stay down for ENTRIES_WAKE_HOLD frames (VCOUNT counted, the
    // screen still off); released earlier, even for a single frame (a
    // bounce), the GBA stops again with nothing redrawn. Woken by anything
    // but START (an emulator without Stop returns at once) it stops again
    // too. The core_lock's end waits for VBlank and turns the display back
    // on: VRAM is kept, so the screen is the same.
    while (bn::keypad::start_held()) bn::core::update();
    bn::core_lock lock;
    reg16(DISPCNT) = uint16_t(reg16(DISPCNT) | DISPCNT_BLANK);
    for (;;) {
        while (start_down()) {}
        reg16(KEYCNT) = uint16_t(0x4000 | START_BIT);
        keypad_irq(true);
#if defined(__thumb__)
        __asm__ volatile("swi 0x03" ::: "r0", "r1", "r2", "r3", "memory");
#else
        __asm__ volatile("swi 0x030000" ::: "r0", "r1", "r2", "r3", "memory");
#endif
        keypad_irq(false);
        unsigned frames = 0;
        bool vblank = reg16(VCOUNT) >= 160;
        while (start_down() && frames < ENTRIES_WAKE_HOLD) {
            bool now = reg16(VCOUNT) >= 160;
            if (now && !vblank) ++frames;
            vblank = now;
        }
        if (frames >= ENTRIES_WAKE_HOLD) break;
    }
    reg16(KEYCNT) = 0;
}
#endif

int run_menu(Canvas& c, const char* title, const char* const* items, const char* const* confirm, int count, int selected)
{
    Input input;
    input.reset(list_keys());
    int asking = -1;
    bool redraw = true;
    if (selected < 0 || selected >= count) selected = 0;
    for (;;) {
        Steps s = input.frame(list_keys());
        if (s.pressed & entries::B) {
            if (asking < 0) return -1;
            asking = -1; redraw = true;
        } else if (s.pressed & entries::A) {
            if (asking == selected || !confirm || !confirm[selected]) return selected;
            asking = selected; redraw = true;
        } else if (s.rows[0] || s.rows[1]) {
            int d = s.rows[1] ? 1 : -1;
            selected = (selected + count + d) % count;
            asking = -1; redraw = true;
        }
        if (!redraw) { bn::core::update(); continue; }
        redraw = false;
        c.clear();
        c.ui_center(0, title);
        for (int i = 0; i < count; ++i)
            draw_row(c, ROW_Y + i * ROW_H, asking == i ? confirm[i] : items[i], i == selected, 0);
        c.flip();
    }
}

void entries_show_saving(Canvas& c)
{
    const char* text = save_status_text(SaveStatus::SAVING);
    c.ui(236 - 6 * int(std::strlen(text)), 0, text);   // same place as on the learning screen
    bn::core::update();
}

void entries_draw_menu(Canvas& c, const char* title, const char* const* items, int count, int selected, const char* footer)
{
    c.clear();
    c.ui_center(0, title);
    for (int i = 0; i < count && i < ROWS; ++i) draw_row(c, ROW_Y + i * ROW_H, items[i], i == selected, 0);
    if (footer && footer[0]) c.body_from(TEXT_X, ROW_Y + ROWS * ROW_H - ROW_H + 1, footer, 0, TEXT_RIGHT - TEXT_X, ui_colors::GREY);
}

void entries_draw_tick(Canvas& c, int y, const char* text)
{
    // gbareader's tick (reader_screen.cpp), 8 x 6, 5 px after the text.
    static constexpr uint8_t rows[6] = {0x01, 0x03, 0x86, 0xCC, 0x78, 0x30};
    const int x = TEXT_X + int(ui_text_width(text)) + 5;
    for (int j = 0; j < 6; ++j)
        for (int i = 0; i < 8; ++i)
            if (rows[j] & (0x80 >> i)) c.fill(x + i, y + 5 + j, 1, 1, ui_colors::BLACK);
}

// gbareader page lines: one per list row from first_row, "#" lines are grey
// subheadings, empty lines stay empty.
static void draw_lines(Canvas& c, const char* const* lines, int count, int first_row)
{
    for (int i = 0; i < count && first_row + i < ROWS; ++i) {
        const char* line = lines[i];
        if (!line || !line[0]) continue;
        const int y = ROW_Y + (first_row + i) * ROW_H;
        if (line[0] == '#') c.ui_grey(TEXT_X, y + 1, line + 1);
        else c.body_from(TEXT_X, y + 1, line, 0, TEXT_RIGHT - TEXT_X);
    }
}

void draw_message(Canvas& c, const char* title, const char* const* lines, int count)
{
    c.clear();
    c.ui_center(0, title);
    draw_lines(c, lines, count, 0);
    c.flip();
}

void run_notice(Canvas& c, const char* title, const char* const* lines, int count)
{
    Input input;
    input.reset(list_keys());
    draw_message(c, title, lines, count);
    for (;;) {
        Steps s = input.frame(list_keys());
        if (s.pressed & (entries::A | entries::B)) return;
        bn::core::update();
    }
}

int run_choice(Canvas& c, const char* title, const char* const* lines, int line_count,
               const char* const* items, int item_count, int selected)
{
    Input input;
    input.reset(list_keys());
    if (selected < 0 || selected >= item_count) selected = 0;
    bool redraw = true;
    for (;;) {
        Steps s = input.frame(list_keys());
        if (s.pressed & entries::B) return -1;
        if (s.pressed & entries::A) return selected;
        if (s.rows[0] || s.rows[1]) {
            selected = (selected + item_count + (s.rows[1] ? 1 : -1)) % item_count;
            redraw = true;
        }
        if (!redraw) { bn::core::update(); continue; }
        redraw = false;
        entries_draw_choice(c, title, lines, line_count, items, item_count, selected);
        c.flip();
    }
}

void entries_draw_choice(Canvas& c, const char* title, const char* const* lines, int line_count,
                         const char* const* items, int item_count, int selected)
{
    c.clear();
    c.ui_center(0, title);
    draw_lines(c, lines, line_count, 0);
    for (int i = 0; i < item_count && line_count + i < ROWS; ++i)
        draw_row(c, ROW_Y + (line_count + i) * ROW_H, items[i], i == selected, 0);
}
