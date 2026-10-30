#pragma once
// Shared mode-4 canvas of the menu, dictionary and entry list screens: white
// background, black text, light-blue selected row (gbamp3's colours).
#include <cstdint>
#include <cstring>

namespace ui_colors {
constexpr uint8_t WHITE = 0, BLACK = 1, GREY = 2, LIGHT = 3, BLUE = 4, BOX = 5;
}

#ifdef UI_CANVAS_HOST
// Host double: text is logged, pixels are ignored.
extern void (*ui_canvas_log)(const char*);
struct Canvas {
    uint8_t* pixels = nullptr;
    void clear() { if (ui_canvas_log) ui_canvas_log(nullptr); }
    void ui(int, int, const char* s) { if (ui_canvas_log) ui_canvas_log(s); }
    void ui_grey(int, int, const char* s, bool = false) { if (ui_canvas_log) ui_canvas_log(s); }
    void ui_center(int, const char* s) { if (ui_canvas_log) ui_canvas_log(s); }
    void body(int, int, const char* s, int = 224) { if (ui_canvas_log) ui_canvas_log(s); }
    void body_from(int, int, const char* s, unsigned, int, uint8_t = ui_colors::BLACK) { if (ui_canvas_log) ui_canvas_log(s); }
    void fill(int, int, int, int, uint8_t) {}
    void glyph2x(int, int, const char*, uint8_t) { if (ui_canvas_log) ui_canvas_log("[letter box]"); }
    void flip();
    void caret(int, int) {}
};
#else
#include "bn_bg_palette_item.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_palette_bitmap_bg_painter.h"
#include "bn_palette_bitmap_bg_ptr.h"
#include "bn_sprite_items_ui_font.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_font.h"
#include "bn_sprite_palette_item.h"
#include "ui_sprite_font.h"
extern "C" {
#include "../references/gbawriter/src/fonts/font_render.h"
}
namespace ui_colors {
constexpr bn::color palette_colors[16] = {
    bn::color(31, 31, 31), bn::color(0, 0, 0), bn::color(12, 12, 12), bn::color(20, 20, 20),
    bn::color(21, 26, 31),   // RGB(170,210,255): gbamp3's selected row
    bn::color(8, 8, 8)};     // letter box
constexpr bn::bg_palette_item palette(bn::span<const bn::color>(palette_colors), bn::bpp_mode::BPP_8);
}
struct Canvas {
    bn::palette_bitmap_bg_ptr bg = bn::palette_bitmap_bg_ptr::create(ui_colors::palette);
    bn::palette_bitmap_bg_painter painter{bg};
    bn::sprite_font font{bn::sprite_items::ui_font, ui::sprite_font_utf8_characters_map.reference(), ui::sprite_font_character_widths};
    bn::sprite_text_generator generator{font};
    bn::vector<bn::sprite_ptr, 128> sprites;
    uint8_t* pixels = nullptr;
    Canvas() { generator.set_palette_item(bn::sprite_items::ui_font.palette_item()); generator.set_left_alignment(); }
    void clear() { sprites.clear(); painter.fill(0); pixels = reinterpret_cast<uint8_t*>(painter.page().data()); }
    void ui(int x, int y, const char* s) { generator.generate(x - 120, y - 72, s, sprites); }
    void ui_center(int y, const char* s) {
        generator.set_center_alignment(); generator.generate(0, y - 72, s, sprites); generator.set_left_alignment();
    }
    // Small grey text (subheadings, page numbers); right: x is the right end.
    void ui_grey(int x, int y, const char* s, bool right = false) {
        generator.set_palette_item(grey_palette());
        if (right) generator.set_right_alignment();
        generator.generate(x - 120, y - 72, s, sprites);
        generator.set_left_alignment();
        generator.set_palette_item(bn::sprite_items::ui_font.palette_item());
    }
    void body(int x, int y, const char* s, int width = 224) { draw_text_idx8_bus16_range(s, pixels + y * 240 + x, 0, width, 240, 1); }
    // skip: pixels of the text scrolled out on the left (marquee).
    void body_from(int x, int y, const char* s, unsigned skip, int width, uint8_t color = ui_colors::BLACK) {
        draw_text_idx8_bus16_range(s, pixels + y * 240 + x, skip, width, 240, color);
    }
    void flip() { painter.flip_page_later(); bn::core::update(); }
    // Halfword stores: GBA VRAM has no byte writes.
    void fill(int x, int y, int w, int h, uint8_t c) {
        if (x < 0) { w += x; x = 0; }
        if (x + w > 240) w = 240 - x;
        for (int j = 0; j < h && y + j < 160; ++j) {
            uint8_t* row = pixels + (y + j) * 240;
            int i = x;
            if (i & 1 && i < x + w) { put(row, i, c); ++i; }
            auto* p = reinterpret_cast<volatile uint16_t*>(row + i);
            uint16_t both = uint16_t(c | (c << 8));
            for (; i + 1 < x + w; i += 2) *p++ = both;
            if (i < x + w) put(row, i, c);
        }
    }
    // One glyph at twice its size (the letter box).
    void glyph2x(int x, int y, const char* ch, uint8_t c) {
        alignas(2) uint8_t glyph[16 * 16] = {};
        draw_text_idx8_bus16_range(ch, glyph, 0, 16, 16, 1);
        for (int gy = 0; gy < 16; ++gy) for (int gx = 0; gx < 16; ++gx)
            if (glyph[gy * 16 + gx]) fill(x + gx * 2, y + gy * 2, 2, 2, c);
    }
    void caret(int x, int y) {
        if (x > 230) x = 230;
        for (int i = 0; i < 16; ++i) put(pixels + (y + i) * 240, x, 1);
    }
private:
    // The UI font's palette with its ink (index 12) in the canvas grey.
    static const bn::sprite_palette_item& grey_palette() {
        static bn::color colors[16];
        static bool made = false;
        if (!made) {
            const auto source = bn::sprite_items::ui_font.palette_item().colors_ref();
            for (int i = 0; i < 16 && i < source.size(); ++i) colors[i] = source[i];
            colors[12] = bn::color(12, 12, 12);
            made = true;
        }
        static const bn::sprite_palette_item item(bn::span<const bn::color>(colors), bn::bpp_mode::BPP_4);
        return item;
    }
    static void put(uint8_t* row, int x, uint8_t c) {
        auto* p = reinterpret_cast<volatile uint16_t*>(row + (x & ~1));
        *p = (x & 1) ? uint16_t((*p & 0x00ff) | (c << 8)) : uint16_t((*p & 0xff00) | c);
    }
};
#endif

// Pixel width of UI body text (SuperFW font); the host double uses 8 px.
unsigned ui_text_width(const char* s);
// UTF-8 bytes of s that fit in width pixels.
unsigned ui_text_fit(const char* s, unsigned width);
