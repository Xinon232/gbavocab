#pragma once
#include <cstdint>
class Renderer;
struct Canvas;
// V4.4.3: two language codes typed like Import's "Language codes 1/2, 2/2"
// (lowercase a-z 0-9 -, max 11, different). for_list: "Save" on step 2.
bool ask_language_codes(Canvas& c, char first[12], char second[12], bool for_list);
// Start on the home screen (V4.1): browse the card, import TXT lists and
// .dict files into /gbavocab, or build a .dict from a TXT. Returns when B is
// pressed at the card root.
void run_import_screen(Renderer& renderer);
// "70,112"
void import_number(char* out, uint32_t value);
