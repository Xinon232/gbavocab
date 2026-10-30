#include "flashcard_turn.h"
#include "bn_assert.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_shape_size.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_tile.h"

namespace {

bn::sprite_shape turned_shape(bn::sprite_shape shape)
{
    if (shape == bn::sprite_shape::WIDE) return bn::sprite_shape::TALL;
    if (shape == bn::sprite_shape::TALL) return bn::sprite_shape::WIDE;
    return shape;
}

// One 4 bpp tile turned about its diagonal: out pixel (x, y) = in (y, x).
// Row r of a tile is a word, pixel x its nibble x.
void transpose_tile(const uint32_t* in, volatile uint32_t* out)
{
    uint32_t rows[8];
    for (int r = 0; r < 8; ++r) rows[r] = in[r];
    for (int r = 0; r < 8; ++r) {
        const int shift = r * 4;
        out[r] = ((rows[0] >> shift) & 15) | ((rows[1] >> shift) & 15) << 4 |
                 ((rows[2] >> shift) & 15) << 8 | ((rows[3] >> shift) & 15) << 12 |
                 ((rows[4] >> shift) & 15) << 16 | ((rows[5] >> shift) & 15) << 20 |
                 ((rows[6] >> shift) & 15) << 24 | ((rows[7] >> shift) & 15) << 28;
    }
}

// The sprite's own tiles turned about the diagonal, in place: pixel (X, Y)
// becomes the old (Y, X), on a sprite tall for wide. No new tiles, so a
// turned card takes no more sprite memory than an upright one. Tiles shared
// by several sprites (the notice strip) are turned once.
void transpose_in_place(bn::sprite_ptr& sprite, const bn::tile** done, int& done_count)
{
    const bn::sprite_shape_size shape = sprite.shape_size();
    const int w = shape.width(), h = shape.height();
    const bn::sprite_shape_size turned(turned_shape(shape.shape()), shape.size());
    bn::sprite_tiles_ptr tiles = sprite.tiles();
    bn::span<bn::tile> vram = *tiles.vram();
    bool seen = false;
    for (int i = 0; i < done_count && !seen; ++i) seen = done[i] == vram.data();
    if (!seen) {
        // The learning screen's sprites are at most 32 x 16 (8 tiles).
        BN_ASSERT(vram.size() <= 8, "Turned sprite too big: ", vram.size());
        bn::tile source[8];
        for (int t = 0; t < vram.size(); ++t) source[t] = vram[t];
        // The new sprite is h wide and w high: its tile (tx, ty) is the old
        // tile (ty, tx), turned. VRAM: word stores only.
        const int columns = w >> 3, rows = h >> 3;
        for (int ty = 0; ty < columns; ++ty)
            for (int tx = 0; tx < rows; ++tx)
                transpose_tile(source[tx * columns + ty].data, vram[ty * rows + tx].data);
        if (done_count < 128) done[done_count++] = vram.data();
    }
    // A new sprite on the same tiles (set_tiles keeps the old shape when the
    // tiles do not change).
    bn::sprite_builder builder(turned, bn::move(tiles), sprite.palette());
    builder.set_z_order(sprite.z_order());
    builder.set_bg_priority(sprite.bg_priority());
    sprite = builder.release_build();
}

}  // namespace

bn::sprite_ptr sprite_with_own_tiles(const bn::sprite_item& item, int x, int y)
{
    const bn::span<const bn::tile> source = item.tiles_item().graphics_tiles_ref();
    bn::sprite_tiles_ptr tiles = bn::sprite_tiles_ptr::allocate(source.size(), bn::bpp_mode::BPP_4);
    for (int t = 0; t < source.size(); ++t) {
        volatile uint32_t* out = tiles.vram()->data()[t].data;
        for (int r = 0; r < 8; ++r) out[r] = source[t].data[r];
    }
    return bn::sprite_ptr::create(x, y, item.shape_size(), bn::move(tiles),
                                  bn::sprite_palette_ptr::create(item.palette_item()));
}

void turn_sprites(bn::ivector<bn::sprite_ptr>& sprites, int first, FlashcardAngle angle)
{
    if (angle == FlashcardAngle::normal) return;
    const bn::tile* done[128];
    int done_count = 0;
    for (int i = first; i < sprites.size(); ++i) {
        bn::sprite_ptr& sprite = sprites[i];
        const bn::fixed x = sprite.x(), y = sprite.y();
        if (angle == FlashcardAngle::upside_down) {
            // Pixel (x, y) goes to (-1 - x, -1 - y): the centre to (-x, -y).
            sprite.set_position(-x, -y);
            sprite.set_horizontal_flip(!sprite.horizontal_flip());
            sprite.set_vertical_flip(!sprite.vertical_flip());
            continue;
        }
        transpose_in_place(sprite, done, done_count);
        if (angle == FlashcardAngle::right) {
            // gbareader's 90° right: pixel (x, y) goes to (y, -1 - x).
            sprite.set_position(y, -x);
            sprite.set_vertical_flip(true);
        } else {
            // 90° left: pixel (x, y) goes to (-1 - y, x).
            sprite.set_position(-y, x);
            sprite.set_horizontal_flip(true);
        }
    }
}
