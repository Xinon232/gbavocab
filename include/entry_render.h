#pragma once
#include "entry_editor.h"
// UI text at x, y; x < 0: centred (headers).
using EntryUiLine = void (*)(void*, int, int, const char*);
int entry_glyph_width(const char* text);
void render_entry(EntryEditor& editor, uint8_t* pixels, EntryUiLine ui, void* context);
// Entry editor menu row i (drawn as a gbamp3 list by the entry screen).
const char* entry_menu_item(const EntryEditor& editor, int i);
