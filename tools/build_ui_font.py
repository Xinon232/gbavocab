#!/usr/bin/env python3
"""Build the UI sprite font from the gbamp3 5x7 glyphs.

Every glyph the gbamp3 v0.9.8 player draws (letters with g j p q y reaching two
rows below the baseline, digits and its punctuation) comes from its own code,
copied verbatim to tools/gbamp3_small_font/small_font.c and compiled here
(github.com/Xinon232/gbamp3, player/src/text.c, MIT, see
references/gbamp3-font/LICENSE). ASCII characters gbamp3 leaves blank and the
Spanish accented letters (gbamp3 shows bare base letters) and '°' are drawn
here in the same 5x7 style. V4.4.3: '>' is gbamp3's glyph too (it shows in learning,
"de > en"); the old blue Butano glyph is gone. Output: graphics/ui_font.bmp (8x16 cells, same
16-colour palette as before, glyphs in black) and include/ui_sprite_font.h.

Usage: python3 tools/build_ui_font.py   (deterministic; needs a host C compiler)
"""
import os
import subprocess
import tempfile
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
UTF8 = 'ÁÉÍÓÚÜÑáéíóúüñ¡¿°'
ADVANCE = 6          # gbamp3 draws every character 6 px apart
TOP = 5              # glyph row 0 sits on cell row 5, like the old capitals
DESCENT = 2          # gbamp3 v0.9.8: g j p q y use two rows below the baseline
BLACK = 12           # palette index of (0, 0, 0)

# Previous UI palette (16 colours; the glyphs use only black).
PALETTE = [
    (0, 255, 0), (49, 65, 95), (108, 56, 90), (176, 50, 67),
    (225, 57, 57), (96, 97, 97), (73, 101, 130), (240, 101, 70),
    (98, 131, 191), (97, 142, 188), (255, 146, 83), (134, 184, 165),
    (0, 0, 0), (188, 193, 195), (198, 247, 255), (15, 0, 0),
]



def gbamp3_glyphs():
    """Rows (5 bits each, 16 = leftmost) of every glyph gbamp3 v0.9.8 draws."""
    source = ROOT / 'tools' / 'gbamp3_small_font' / 'small_font.c'
    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, 'small_font')
        subprocess.run([os.environ.get('CC', 'cc'), '-O0', '-o', exe, str(source)], check=True)
        out = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    glyphs = {}
    for line in out.splitlines():
        code, *rows = (int(v) for v in line.split())
        assert len(rows) == 7 + DESCENT
        if any(rows):
            # Seven rows unless the glyph uses the descender rows.
            glyphs[chr(code)] = rows if any(rows[7:]) else rows[:7]
    return glyphs


GLYPHS = gbamp3_glyphs()
GBAMP3_CHARACTERS = sorted(GLYPHS)
DRAWN = {  # drawn for gbavocab in the same style, used where gbamp3 has no glyph
    ' ': [0]*7, '!': [4,4,4,4,4,0,4], '"': [10,10,0,0,0,0,0], '#': [10,10,31,10,31,10,10],
    '$': [4,15,20,14,5,30,4], '%': [24,25,2,4,8,19,3], '&': [12,18,20,8,21,18,13],
    "'": [4,4,0,0,0,0,0], '*': [0,4,21,14,21,4,0], '+': [0,4,4,31,4,4,0],
    ';': [0,0,4,0,0,4,8], '<': [0,2,4,8,4,2,0], '=': [0,0,31,0,31,0,0],
    '?': [14,17,1,2,4,0,4], '[': [14,8,8,8,8,8,14], '\\': [16,8,4,2,1,1,1],
    ']': [14,2,2,2,2,2,14], '^': [4,10,17,0,0,0,0], '`': [8,4,0,0,0,0,0],
    '{': [6,4,4,8,4,4,6], '|': [4,4,4,4,4,4,4], '}': [12,4,4,2,4,4,12],
    '~': [0,0,8,21,2,0,0], '¡': [4,0,4,4,4,4,4], '¿': [4,0,4,8,16,17,14],
    '°': [12,18,18,12,0,0,0],   # Settings > Flashcard angle ("90° right")
}
for character, rows in DRAWN.items():
    GLYPHS.setdefault(character, rows)
# Accented letters: capitals get the mark on the two rows above (cell rows 3-4);
# lowercase letters use their own empty top rows.
ACUTE, UMLAUT, TILDE = [2, 4], [0, 10], [13, 22]
for base, mark, target in (('A', ACUTE, 'Á'), ('E', ACUTE, 'É'), ('I', ACUTE, 'Í'), ('O', ACUTE, 'Ó'),
                           ('U', ACUTE, 'Ú'), ('U', UMLAUT, 'Ü'), ('N', TILDE, 'Ñ')):
    GLYPHS[target] = (mark, GLYPHS[base])
for base, mark, target in (('a', ACUTE, 'á'), ('e', ACUTE, 'é'), ('o', ACUTE, 'ó'), ('u', ACUTE, 'ú'),
                           ('u', UMLAUT, 'ü'), ('n', TILDE, 'ñ')):
    GLYPHS[target] = mark + GLYPHS[base][2:]
GLYPHS['í'] = [2, 4] + GLYPHS['i'][2:]

ORDER = [chr(c) for c in range(33, 127)] + list(UTF8)


def draw(image, index, character):
    y0 = index * 16
    glyph = GLYPHS[character]
    mark = []
    if isinstance(glyph, tuple):
        mark, glyph = glyph
    assert len(glyph) in (7, 7 + DESCENT) and all(0 <= r < 32 for r in glyph), character
    for top, rows in ((TOP - len(mark), mark), (TOP, glyph)):
        for y, bits in enumerate(rows):
            for x in range(5):
                if bits & (16 >> x):
                    image.putpixel((x, y0 + top + y), BLACK)


def main():
    image = Image.new('P', (8, 16 * len(ORDER)), 0)
    image.putpalette([v for color in PALETTE for v in color])  # 16 entries: a 4 bpp sprite palette
    for index, character in enumerate(ORDER):
        draw(image, index, character)
    image.save(ROOT / 'graphics' / 'ui_font.bmp')
    (ROOT / 'graphics' / 'ui_font.json').write_text('{\n    "type": "sprite",\n    "height": 16\n}\n')
    # Name space and backslash in words: a comment ending in '\\' would
    # continue onto (and swallow) the next line.
    names = {32: 'space', 92: 'backslash'}
    widths = '\n'.join(f'    {ADVANCE},  // {code} {names.get(code, chr(code))}' for code in range(32, 127))
    widths += '\n' + '\n'.join(f'    {ADVANCE},  // {c}' for c in UTF8)
    characters = ', '.join(f'"{c}"' for c in UTF8)
    (ROOT / 'include' / 'ui_sprite_font.h').write_text(f'''// Generated by tools/build_ui_font.py: gbamp3 5x7 UI glyphs (MIT) in an
// 8x16 Butano sprite font, 6 px per character like gbamp3.
#ifndef UI_SPRITE_FONT_H
#define UI_SPRITE_FONT_H

#include "bn_sprite_font.h"
#include "bn_utf8_characters_map.h"
#include "bn_sprite_items_ui_font.h"

namespace ui
{{

constexpr bn::utf8_character sprite_font_utf8_characters[] = {{
    {characters}
}};

constexpr int8_t sprite_font_character_widths[] = {{
{widths}
}};

constexpr bn::span<const bn::utf8_character> sprite_font_utf8_characters_span(
        sprite_font_utf8_characters);

constexpr auto sprite_font_utf8_characters_map =
        bn::utf8_characters_map<sprite_font_utf8_characters_span>();

constexpr bn::sprite_font sprite_font(
        bn::sprite_items::ui_font, sprite_font_utf8_characters_map.reference(),
        sprite_font_character_widths);

}}

#endif
''', encoding='utf-8')


if __name__ == '__main__':
    main()
