# gbavocab 11.0

Based on gbavocab 10.0. The source code of this release is published at github.com/Xinon232/gbavocab (with the gbaSD submodule).

## Large SD cards

- Every SD card the carts can address works, FAT32 and exFAT, up to 2 TB (the largest SDXC card). This is now tested with gbaSD's large-card kit on the SuperCard, EZ-Flash Omega (also with the kernel's in-game interrupt hook) and EverDrive GBA models: exFAT 64 GB and 2 TB (128 KB and 512 KB clusters), exFAT with large clusters (64 GB with 1 MB, 2 TB with 32 MB, the largest exFAT allows) and FAT32 32 GB, with `/gbavocab` at the far end of the card. The app starts, opens a dictionary and changes it, opens and saves a list, deletes an entry and saves Settings exactly as on a small card.
- The app's own size arithmetic was checked: it never asks for the card size or the free space, and file sizes are compared in 64 bits. A `.dict` of 4 GB or more (possible on exFAT) had its size cut to 32 bits and could open as a smaller dictionary; it is now not opened.
- The first save after a start is fast on every card. exFAT keeps no note of where free space is, so the free space was searched from the start of the card, bit by bit: on a nearly full card whose free space is at its end, the first save took up to about 83 seconds on the SuperCard model (2 TB, 128 KB clusters; 58 s on the Omega and EverDrive models) and 2 to 3 seconds on a 64 GB card. gbaSD's FatFs now searches a word at a time, and the app starts the search at the end of its own files in `/gbavocab` (gbaSD's `gbasd_alloc_hint`): the first save takes at most 25 frames (0.4 s) on every tested card and model, as on a small card.
- exFAT cards with 32 MB clusters can be used (FatFs refused them before). On such a card the first start without `/gbavocab/GBAVOCAB.SYS` takes about 2 to 3 minutes on the models (108 s SuperCard, 173 s Omega, 190 s EverDrive) while the app makes that folder (a new folder clears a whole cluster, 32 MB); later starts are not slowed.

## Licence

- The UI font (the 5x7 glyphs from gbamp3, by the same author) is GPL-3.0-or-later like the rest of gbavocab; it was credited as MIT before. Its notice in `references/gbamp3-font/LICENSE` points to `LICENSE`.

## Notes

- gbaSD (git submodule) is at the version with the large-card test kit, the fast exFAT free-space search, `gbasd_alloc_hint` and 32 MB clusters. Its optional multi-sector buffer (`GBASD_FF_MULTI_BUF`) stays off: its 2 KB do not fit the app's memory reserve.
- No change to lists, `GBAVOCAB.SYS` or the `.dict` format (version 4). The PC dictionary builder stays at 2.0.0.
