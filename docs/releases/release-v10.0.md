# gbavocab 10.0

Based on gbavocab V4.4.3. All apps of the series are now numbered 10.0. The source code of this release is published at github.com/Xinon232/gbavocab (with the gbaSD submodule).

## Flashcarts and SD cards

- One ROM for the **SuperCard SD**, the **EZ-Flash Omega** (also the Definitive Edition) and the **EverDrive GBA**, with the SD card code from [gbaSD](https://github.com/Xinon232/gbaSD) (git submodule). The app checks at every start which cart it is on; hold L+R while it starts to use the SuperCard driver.
- FAT (FAT16/FAT32) and exFAT cards. FatFs now comes from gbaSD (`gbasd/fatfs`) with its power-cut fixes: a folder growing on a fragmented exFAT card, a folder cluster on FAT, and a file with a long name deleted or renamed on FAT (no 8.3 alias name left visible).
- exFAT: entry sets a power cut left half written are repaired at every start (root, `/gbavocab`, `GBAVOCAB.SYS`).

## Settings

- **Select** in the main menu opens **Settings** (called Help before): New List, Flashcard angle, Save settings and Hide elements first, then the help topics and Credits.
- **Flashcard angle**: Normal, 90° right, 90° left or Upside down for the learning screen; the D-pad turns with it. At 90° long cards keep their size more often.
- **Hide elements**: hide the asked language, the line `Box N/5 - T words` or the box counts; nothing else moves.

## Main menu sleep

- **L+R** together in the main menu turns the screen off and stops the GBA; **Start held 1 s** wakes it on the same screen (a shorter press leaves it asleep).

## Credits

- Settings > Credits, first page: gbavocab 10.0, (c) 2026 Halim Jarrar, GPL-3.0-or-later, no warranty, the source code link github.com/Xinon232/gbavocab and the contact. The next pages credit the parts made by others with their licenses.
- The manual PDF ends with an About / licence page with the same information.

## Notes

- No change to lists, `GBAVOCAB.SYS` or the `.dict` format (version 4). The PC dictionary builder stays at 2.0.0.
