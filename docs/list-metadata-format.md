# List settings: GBAVOCAB.SYS (V4.0)

Since V4.0 all list settings live in one central configuration, modelled on
gbamp3's two-slot state files. Per-list `.sav` files (V1.6–V3.0) are no longer
read or written; there is no migration from them. TXT lists contain only
vocabulary rows and box separators and stay dict.cc compatible.

## Location

```
/gbavocab/GBAVOCAB.SYS/CONFIG0.DAT
/gbavocab/GBAVOCAB.SYS/CONFIG1.DAT
```

The folder is created on the first settings save. It is application-managed SD
data, not cartridge SRAM. LOAD LIST and the dictionary catalog ignore it.
Deleting the folder resets every list's settings and the last-list memory;
lists and dictionaries are not affected.

## What is stored

- For each list, by TXT filename (compared case-insensitively, like FAT):
  FRONT / BACK language codes, learning mode (1 front, 2 back, 3 alternate)
  and the linked dictionary filename.
- The last opened list (LOAD LIST starts on it after a restart).

Up to 192 lists are remembered. When the table is full, the list that was used
longest ago is dropped. Renaming a TXT on a PC starts it with default settings
(Alternate, no pair, no link).

## When it is written

- Opening or creating a list: the last opened list.
- Choosing a list's language pair or linking a dictionary: at once.
- Manual save (learning Start, or Save when switching lists): the list's pair,
  mode and link, before the TXT is written. A settings-only save does not
  rewrite the TXT.

Unchanged settings are not rewritten.

## Binary layout (both slots)

Little endian.

| Offset | Size | Field |
|---:|---:|---|
| 0 | 8 | magic `GBVCFG01` |
| 8 | 4 | generation |
| 12 | 4 | record count *n* (≤ 192) |
| 16 | 4 | use clock |
| 20 | 12 | reserved, zero |
| 32 | 64 | last opened list, NUL padded |
| 96 | 160 × *n* | records |
| 96 + 160 *n* | 4 | FNV-1a 32 of all preceding bytes |

Record (160 bytes):

| Offset | Size | Field |
|---:|---:|---|
| 0 | 64 | list filename, NUL padded |
| 64 | 12 | front language code, NUL padded (empty: no pair) |
| 76 | 12 | back language code, NUL padded |
| 88 | 64 | linked dictionary filename (`*.dict`), or empty |
| 152 | 1 | learning mode 1–3 |
| 153 | 3 | zero |
| 156 | 4 | last use (use clock value) |

Language codes are 1–11 lowercase ASCII letters, digits or hyphens starting
with a letter, and the two codes differ.

## Two slots

A save writes the slot that is not the newest, with generation + 1, then syncs
and closes it. Reading takes the slot with the higher generation whose size,
magic, record count and checksum are valid; if the newer slot is damaged the
older one is used. An interrupted save therefore keeps the previous settings.
Records that fail validation are dropped on the next save; the rest are kept.

## Legacy TXT footer

A last line such as `# gbavocab: front=en; back=de` (written by early versions)
is still read as the list's pair when the configuration has none, and removed
from the TXT on the next save. A remembered pair wins over a footer.

## Source

`include/app_config.h`, `src/app_config.cpp`; wiring in `src/vocab_file_io.cpp`
(`vocab_file_save_settings`, `vocab_file_remember_last`,
`vocab_file_last_list_index`). Host tests: `tests/test_app_config.cpp`,
`tests/test_list_metadata*.cpp`, `tests/test_list_settings.cpp`.
