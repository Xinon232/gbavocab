#pragma once
// Settings > Flashcard angle: turns the learning screen's sprites (GBA only;
// render.cpp lays them out for the turned screen first).
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "flashcard_angle.h"

// Sprites from first on: Upside down flips each sprite and mirrors its place
// through the screen centre. 90° turns each sprite's pixels in its own tiles
// (the shape tall for wide) and turns its place: no new sprite memory, and no
// affine sprites, which would take twice the scanline time of the card's
// text. Every sprite's tiles must be allocated (sprite_tiles_ptr::allocate).
void turn_sprites(bn::ivector<bn::sprite_ptr>& sprites, int first, FlashcardAngle angle);

// item.create_sprite(x, y), but with allocated tiles (a copy of the item's
// first graphic), so turn_sprites can read them.
bn::sprite_ptr sprite_with_own_tiles(const bn::sprite_item& item, int x, int y);
