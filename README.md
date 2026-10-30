# gbavocab 10.0

Learn vocabulary with flashcards and five learning boxes, and create and edit your own word lists on your Game Boy Advance (SuperCard SD, EZ-Flash Omega or EverDrive GBA). Lists are plain dict.cc-style TXT files on the SD card; local dictionaries are indexed `.dict` files, built on a PC or on the GBA itself.

This repository holds the complete source code of gbavocab 10.0. The ROM (free), the PC dictionary builder and the manual are available at **[halimj.itch.io](https://halimj.itch.io)**. The manual is also in this repository: [docs/gbavocab-full-controls.pdf](docs/gbavocab-full-controls.pdf) (source: [docs/full-controls.md](docs/full-controls.md)). What is new: [docs/releases/release-v10.0.md](docs/releases/release-v10.0.md).

## Using it

1. Copy `gbavocab.gba` to your card and start it with your firmware. The app checks at every start which cart it is on; hold **L+R** while it starts to use the SuperCard driver. The SD card can be FAT (FAT16/FAT32) or exFAT.
2. Put your lists and dictionaries directly in `/gbavocab` at the SD-card root, or press **Start** in the main menu to import them from anywhere on the card. A list row is `word<TAB>translation`; an empty line starts the next learning box (Box 1 to Box 5). [sample file.txt](sample%20file.txt) has 20 English–Spanish entries.
3. Words are typed with the GBAWriter typing system; learn it with [GBAWriter](https://github.com/Xinon232/gbawriter) first.

In the app, **Select** in the main menu opens **Settings** with all help topics and the Credits. Learning: hold R to show the answer, A right, B wrong, Left / Right change the box. Formats: [docs/dictionary-format.md](docs/dictionary-format.md), [docs/list-metadata-format.md](docs/list-metadata-format.md); Arabic display: [docs/arabic.md](docs/arabic.md).

## Building from source

Requirements:

- devkitPro **devkitARM** (the release ROM is built with GCC 16.1.0 and binutils 2.46 from the `devkitpro/devkitarm` Docker image of 2026-06-10, `sha256:15b79ce75822c289538d8153da5fa7aafe5e6adc32ad8a575a197beca0f0761b`), with `DEVKITPRO` and `DEVKITARM` set.
- **[Butano](https://github.com/GValiente/butano)** at commit `77dcbcb3d8783596a9f333c64eedbccec77b05dc`.
- **Python 3** and GNU make.

The SD card code is [gbaSD](https://github.com/Xinon232/gbaSD), included as a git submodule (`gbasd`). Clone with `--recursive`, or run `git submodule update --init` in an existing clone.

```bash
git clone --recursive https://github.com/Xinon232/gbavocab.git
git clone https://github.com/GValiente/butano.git
git -C butano checkout 77dcbcb3d8783596a9f333c64eedbccec77b05dc
cd gbavocab
make LIBBUTANO=../butano/butano
```

The output is `gbavocab.gba` in the repository folder. With the toolchain above it is byte-identical to the released ROM. `make clean LIBBUTANO=../butano/butano` removes the build files.

Generated files are committed, so the build needs nothing else. Their generators are in `tools/`: `tools/build_ui_font.py` (UI font, from `tools/gbamp3_small_font/small_font.c`), `tools/create_field_underline_asset.py` and `tools/extract_ghoulam.py` (Arabic font table, from the Ghoulam TTF; see [docs/arabic.md](docs/arabic.md)). `references/gbawriter/` holds the SuperFW font renderer and font packs taken from GBAWriter, which the build uses.

## PC tools

- **Dictionary builder** (`builder/`): imports UTF-8 TAB-separated exports (TXT / TSV / ZIP, for example from dict.cc), lets you review flagged rows and saves a version-4 `.dict`. Needs Python 3 with tkinter; run `python3 builder/app.py` (`--self-test DIR` runs its self-test). Packaging a standalone app: `python -m pip install PyInstaller==6.16.0 Pillow==12.3.0`, then `python builder/package.py` (output in `builder-dist/`).
- **Manual PDF**: `python3 tools/build_controls_pdf.py --output docs/gbavocab-full-controls.pdf` (needs reportlab, PyMuPDF and the DejaVu fonts in `/usr/share/fonts/truetype/dejavu`).

## License

gbavocab 10.0 · (c) 2026 Halim Jarrar · halimj.itch.io · gba@halim-jarrar.de

gbavocab is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version (GPL-3.0-or-later). It is distributed WITHOUT ANY WARRANTY; see the GNU General Public License in [LICENSE](LICENSE) for details.

## Credits

Parts made by others, in the order of the in-app Credits (Select > Credits):

- Text renderer: [SuperFW](https://github.com/davidgfnet/superfw) by David Guillen Fandos (`references/gbawriter/src`, `include/compiler.h`, `include/gbahw.h`). License: GPL 3.0 or later.
- Fonts: UNSCII by Viznut ([viznut.fi/unscii](https://viznut.fi/unscii/)) and GNU Unifont ([unifoundry.com/unifont](https://unifoundry.com/unifont/)) in the SuperFW font packs (`references/gbawriter/res`). License: GPL 2.0 or later with the font embedding exception ([references/gba-free-fonts-licenses/Unscii.txt](references/gba-free-fonts-licenses/Unscii.txt)).
- Arabic font: Ghoulam Regular (C) 2025 Imad AlFil / mloukhiyye. License: CC BY 4.0. Changed: converted to an 11 px bitmap font — [source, license and extraction details](docs/arabic.md).
- SD card drivers ([gbaSD](https://github.com/Xinon232/gbaSD), see [gbasd/CREDITS.md](gbasd/CREDITS.md)): SuperCard driver from SuperFW by David Guillen Fandos, GPL 3.0 or later; EZ-Flash Omega and EverDrive GBA drivers by Halim Jarrar (GPL 3.0 or later), written after the Omega kernel (Apache 2.0) and edio-gba by krikzz (MIT). File system: FatFs by ChaN, from gbaSD (`gbasd/fatfs`). License: FatFs (BSD style).
- Engine: [Butano](https://github.com/GValiente/butano) by Gustavo Valiente (also `include/common_*` and `src/common_info.cpp` from its examples). License: zlib ([third_party/butano/LICENSE](third_party/butano/LICENSE)). Built with devkitPro / devkitARM.
- UI font: the 5x7 glyphs of gbamp3 by Halim Jarrar (MIT, [references/gbamp3-font/LICENSE](references/gbamp3-font/LICENSE)).

dict.cc is a separate website; gbavocab only reads and writes its TAB-separated vocabulary format. Respect the dict.cc terms for your own exports.
