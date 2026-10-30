#pragma once
#include "vocab.h"
#include "arabic_text.h"

// Accepted rows contain at most 190 display codepoints across both sides.
// Even worst-width word wrapping at 224px needs fewer than 32 spans per side.
// Fail explicitly outside this bound; never return a truncated valid layout.
struct TextLayout {
    uint16_t start[32];
    uint16_t end[32];
    int count = 0;
    bool valid = true;
};
template<class Measure>
TextLayout layout_text(const char* text, int max_width, Measure measure, bool words = true)
{
    TextLayout result;
    int length = int(std::strlen(text)), pos = 0;
    if (length >= VOCAB_LINE_MAX) { result.valid = false; return result; }
    char candidate[VOCAB_LINE_MAX];
    while (pos < length) {
        while (pos < length && text[pos] == ' ') ++pos;
        if (pos == length) break;
        int end = pos, space = -1, batch_end = pos, batch_columns = 32;
        while (end < length) {
            int next = end + 1;
            while (next < length && (static_cast<unsigned char>(text[next]) & 0xc0) == 0x80) ++next;
            std::memcpy(candidate, text + pos, next - pos);
            candidate[next - pos] = 0;
            if (!words) {
                char glyph[5] = {};
                std::memcpy(glyph, text + end, next - end);
                int width = measure(glyph);
                if (text[end] != ' ' && batch_columns + width > 32) {
                    batch_end = end;
                    batch_columns = 0;
                }
                batch_columns += width;
            }
            if (measure(candidate) > max_width) break;
            if (text[end] == ' ') space = end;
            end = next;
        }
        if (end == pos || result.count == 32) { result.valid = false; return result; }
        if (end < length) {
            if (words && space > pos) end = space;
            // Hard-wrap at a 32px sprite-batch boundary when possible. It
            // avoids adding half-filled OAM entries at every wrapped row.
            else if (!words && batch_end > pos) end = batch_end;
        }
        result.start[result.count] = uint16_t(pos);
        result.end[result.count++] = uint16_t(end);
        pos = end;
    }
    return result;
}

// Where a card goes on the learning screen (Butano coordinates, 0 = centre).
// Upright (Normal, Upside down): 240 x 160, the V4.4 layout. Sideways
// (Flashcard angle 90°): the same screen turned, 160 wide and 240 high.
struct CardFrame {
    int width = 224;      // widest text row
    int first_y = -20;    // centre of the prompt's first row
    int answer_y = 12;    // a one-row answer under a one-row prompt
    int bottom = 54;      // lowest edge of the card's rows
    bool sideways = false;
    int tiles = 224;      // sprite tiles one card may use (card_resources)
};
constexpr CardFrame UPRIGHT_CARD{};
// Sideways a card's rows are shorter, so it needs more 32 px sprites: it may
// use 288 tiles (upright 224). Two such cards plus two worst-case UIs stay
// under the 1024 sprite tiles (host renderer test: deferred VRAM peak).
constexpr CardFrame SIDEWAYS_CARD{144, -54, -22, 92, true, 288};

struct CardLayout {
    TextLayout side[2];
    int scale_eighths = 8;
    int line_step = 12;
    int gap = 12;
    bool normal_positions = true;
    int sprite_count = 0;
    bool valid = false;
    CardFrame frame{};
};

inline int card_side_y(const CardLayout& layout, int side, int prompt_side);

// Match Butano's variable 8x16/16x16 painter: 32px batches; spaces advance
// without allocating. Reserve 32 OAM entries for the unchanged UI (host test
// exhausts all legal count strings and every existing save-error notice).
template<class Measure>
bool card_resources(const char* const text[2], Measure measure, CardLayout& layout)
{
    int row_sprites[2][32] = {};
    layout.sprite_count = 0;
    for (int side = 0; side < 2; ++side) {
        for (int row = 0; row < layout.side[side].count; ++row) {
            if (arabic::contains(text[side])) {
                char line[VOCAB_LINE_MAX];
                int n=layout.side[side].end[row]-layout.side[side].start[row];
                std::memcpy(line,text[side]+layout.side[side].start[row],n);line[n]=0;
                int width=measure(side,line);
                row_sprites[side][row]=(width*layout.scale_eighths+255)/256;
                layout.sprite_count+=row_sprites[side][row];
                continue;
            }
            int col = 32, row_width = 0;
            for (int p = layout.side[side].start[row]; p < layout.side[side].end[row];) {
                int end = p + 1;
                while (end < layout.side[side].end[row] && (static_cast<unsigned char>(text[side][end]) & 0xc0) == 0x80) ++end;
                char glyph[5] = {};
                std::memcpy(glyph, text[side] + p, end - p);
                int width = measure(side, glyph);
                if (text[side][p] != ' ' && width && col + width > 32) {
                    ++row_sprites[side][row];
                    ++layout.sprite_count;
                    col = 0;
                }
                col += width;
                row_width += width;
                p = end;
            }
            if (layout.scale_eighths != 8) {
                layout.sprite_count -= row_sprites[side][row];
                row_sprites[side][row] = (row_width * layout.scale_eighths + 255) / 256;
                layout.sprite_count += row_sprites[side][row];
            }
        }
    }
    if (layout.sprite_count > 96) return false;
    // Bound two complete body frames plus two unchanged worst-case UIs.
    if (layout.sprite_count * (layout.scale_eighths == 4 ? 4 : 8) > layout.frame.tiles) return false;
    int width = 32;
    int height = layout.scale_eighths == 4 ? 8 : 16;
    if (layout.frame.sideways) {
        // Turned 90°, a screen line runs across the rows: each row adds at
        // most one sprite, as wide as the row is high.
        int cycles = 0;
        for (int side = 0; side < 2; ++side) cycles += layout.side[side].count * height;
        return cycles <= 960;
    }
    for (int prompt = 0; prompt < 2; ++prompt) {
        for (int y = layout.frame.first_y - 14; y < layout.frame.bottom; ++y) {
            int cycles = 0;
            for (int side = 0; side < 2; ++side) {
                int first = card_side_y(layout, side, prompt);
                for (int row = 0; row < layout.side[side].count; ++row) {
                    int center = first + row * layout.line_step;
                    if (y >= center - height / 2 && y < center + height / 2) cycles += row_sprites[side][row] * width;
                }
            }
            // Non-affine OBJ costs one cycle/pixel. Keep 250 of the GBA's
            // 1210 cycles for UI at the header/body boundary and margin.
            if (cycles > 960) return false;
        }
    }
    return true;
}

// First prompt centre stays at the frame's first_y (upright -20); its bottom (upright 54)
// leaves 2px above the footer.
// Both sides are always measured together, so R/feedback never causes reflow.
// Existing single-line cards keep their exact original font and coordinates.
// Wrapped cards keep full-size typography when possible, but reserve the
// entire cell height so full-height CJK/Arabic glyphs never overlap rows.
template<class Measure>
void layout_card(const char* a, const char* b, Measure measure, CardLayout& result,
                 const CardFrame& frame = UPRIGHT_CARD)
{
    const char* text[2] = {a, b};
    result.valid = false;
    result.frame = frame;
    for (int candidate = 8; candidate >= 3; --candidate) {
        bool words = candidate != 3;
        int scale = words ? candidate : 4;
        for (int side = 0; side < 2; ++side) {
            result.side[side] = layout_text(text[side], frame.width * 8 / scale,
                [&](const char* s) { return measure(side, s); }, words);
        }
        if (!result.side[0].valid || !result.side[1].valid) continue;
        bool normal = scale == 8 && result.side[0].count <= 1 && result.side[1].count <= 1;
        int step = scale * 2; // full 16px source cell, never overlapping dense rows
        // Retry the compact gap at this scale before reducing typography.
        for (int gap : {12, 4}) {
            // Anchor first centre at first_y at EVERY scale. The top cell edge
            // moves with cell height, so the full-size 82px is not a fixed budget.
            int top = frame.first_y - step / 2;
            int bottom = top + (result.side[0].count + result.side[1].count) * step + gap;
            if (!normal && bottom > frame.bottom) continue;
            result.scale_eighths = scale;
            result.gap = normal ? 16 : gap;
            result.line_step = normal ? 12 : step;
            result.normal_positions = normal;
            if (card_resources(text, measure, result)) {
                result.valid = true;
                return;
            }
        }
    }
}

inline int card_side_y(const CardLayout& layout, int side, int prompt_side)
{
    if (layout.normal_positions) {
        return side == prompt_side ? layout.frame.first_y : layout.frame.answer_y;
    }
    int y = layout.frame.first_y; // The active prompt's FIRST line never moves.
    if (side != prompt_side) y += layout.side[prompt_side].count * layout.line_step + layout.gap;
    return y;
}
