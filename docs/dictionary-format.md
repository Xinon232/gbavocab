# gbavocab custom indexed dictionary, version 4 (block index)

`.dict` here is a **custom gbavocab format**, not StarDict, DICT protocol, or a standard dictionary format. Files live directly in SD-root `/gbavocab`. Builder 2.0.0 reads versions 1/2/3/4 and emits version 4 (`GVDIDX04`). **The GBA reader opens only version 4.** Older files are recognized and the screen asks for an upgrade: back up the old file and use PC Open .dict / Save As to a new filename. That resolves all committed additions, replacements and deletions before rebuilding both sorted block indexes. Never patch a header to upgrade. The GBA never rewrites a base or reindexes in place. The chooser discovers up to 24 filenames of at most 63 UTF-8 bytes with case-insensitive `.dict` suffix; internal name/pair is displayed. Files beyond discovery capacity are not selected. Do not modify or swap a file while the application has it open.

Version 4 exists for lookup speed on the SD card. Version 3 needed three scattered small reads (index cell, record, string) for every binary-search step, about a hundred card reads per typed character. Version 4 stores each direction as sorted 4 KiB blocks that are read with one multi-sector card command, and finds the right block through a small directory. The PC builder does the work once; the file is about twice the text size because each direction keeps its own sorted copy of the pairs.

## Integer, string and checksum conventions

All integers are unsigned little-endian 32-bit. Offsets are absolute file offsets. CRC32 is reflected IEEE (polynomial `0xedb88320`, initial `0xffffffff`, final XOR `0xffffffff`), compatible with Python `zlib.crc32`. CRCs detect accidental damage; they are not cryptographic authentication.

Text is strict UTF-8. Each pair is two nonblank fields with no TAB/newline/control inside either field, at most 191 bytes including their separating TAB. Version-1/2 sorting is bytewise UTF-8 with only ASCII A–Z folded to a–z. Versions 3 and 4 use the frozen accent fold below. Equal normalized keys sort by original canonical row ordinal, retaining distinct senses and spellings. The producer validates both sorted permutations and all strings before writing. Original payload bytes are never normalized.

## Immutable indexed base

### Frozen accent fold (versions 3 and 4)

The exact 488-entry mapping is committed in the PC builder and in `include/dictionary_accent_table.h`, with exhaustive parity tests. It covers codepoints in U+00C0–024F and U+1E00–1EFF whose canonical decomposition is one ASCII Latin letter followed only by U+0300–036F combining marks. Each maps to the lowercase ASCII base; ASCII A–Z also maps to a–z. This is a frozen format rule, not a dependency on the operating system's Unicode version.

Process input scalars left to right. After a mapped/ASCII Latin letter, discard successive U+0300–036F marks; keep that Latin context while discarding. Any other retained scalar resets the context unless it is itself an ASCII/mapped Latin letter. All other scalars remain unchanged. Thus composed and decomposed mañana, CAFÉ and ǘ match manana, cafe and u; meaningful Arabic/Indic marks are not dropped. ß/æ/œ/ø/ł are distinct, not ss/ae/oe/o/l. Unsupported canonical decompositions outside these bounded ranges stay exact. This is not universal Unicode casefold, NFKD, transliteration or fuzzy matching.

Sort lexicographically by unsigned folded UTF-8 bytes, then immutable canonical ordinal. Production C++ streams folded scalars without allocation; valid scalar ordering is equivalent to UTF-8 byte ordering. Prefix bounds and mutation rank lookup use exactly this fold, including when a prefix ends after a Latin base but before its combining marks. Malformed dictionary UTF-8 is rejected before comparison; defensive query iteration consumes invalid leading bytes without overread. No normalized spelling is stored in payloads, copied to list drafts or written by mutations. The only folded bytes in a file are the 24-byte directory key prefixes.

The base is at most 64 MiB. Its exact end (not physical EOF) is in the header; additions follow it. Layout, in file order: a 512-byte header sector, the record table, the front directory, the back directory, then (aligned to 4 KiB) the front blocks and the back blocks. Every offset below is derived from N and the two block counts; the reader rejects any other value.

### Header (first 512 bytes)

| Offset | Bytes | Meaning |
|---:|---:|---|
| 0 | 8 | ASCII `GVDIDX04`; PC also reads legacy `GVDIDX01` / `GVDIDX02` / `GVDIDX03` for conversion |
| 8 | 4 | base end / first append-slot offset |
| 12 | 4 | entry count N, 1..16,777,215 |
| 16 | 32 | NUL-terminated name, 1..31 UTF-8 bytes, zero padding |
| 48 | 12 | front language code, zero padding |
| 60 | 12 | back language code, zero padding |
| 72 | 24 | front display label, 1..23 UTF-8 bytes, zero padding |
| 96 | 24 | back display label, same |
| 120 | 4 | block size: exactly 4096 |
| 124 | 4 | record table offset: exactly 512 |
| 128 | 4 | front directory offset: 512 + 4N rounded up to 512 |
| 132 | 4 | front blocks offset |
| 136 | 4 | front block count B0 |
| 140 | 4 | back directory offset: front directory + ceil(B0/15)·512 |
| 144 | 4 | back blocks offset: front blocks + 4096·B0 |
| 148 | 4 | back block count B1 |
| 152 | 4 | CRC32 of bytes [512, base end), checked by the PC reader |
| 156 | 4 | CRC32 of header bytes [0,156) |

Bytes 160–511 are zero. The front blocks start at the end of the back directory rounded up to 4096; base end is back blocks + 4096·B1. Codes are distinct, match `[a-z][a-z0-9-]{0,10}`, and describe canonical columns, never dictionary IDs or the current search direction.

### Canonical rows and the record table

A canonical row is the pair's ordinal in the builder input, 0..N-1; it is the identity used by mutations and it breaks ties between equal keys. The record table holds N little-endian 32-bit values: for row R, its rank in the front-sorted order. The reader verifies that the entry found at that rank names row R.

### Directory sectors

Each direction's directory is ceil(B/15) sectors of 512 bytes: a CRC32 of bytes [4,512) of that sector, then 15 entries of 32 bytes (unused entries and the final 28 bytes are zero). Entry J describes block J:

| Offset | Bytes | Meaning |
|---:|---:|---|
| 0 | 4 | rank of the block's first entry (0 for block 0; ranks are contiguous) |
| 4 | 2 | entries in the block |
| 6 | 1 | stored key length, 1..24 |
| 7 | 1 | 1 if the folded first key is longer than 24 bytes, else 0 |
| 8 | 24 | first 24 bytes of the block's first folded key, zero padding |

The reader binary-searches the directory. A comparison it cannot decide from 24 bytes (a truncated key equal to the query so far, or an exact key tie) reads that block's first entry instead, so long shared prefixes stay correct.

### Blocks

Each block is 4096 bytes: CRC32 of bytes [4,4096), a 16-bit entry count (equal to the directory's), two zero bytes, then one 16-bit offset per entry. Entries follow contiguously in sorted order: a 24-bit little-endian canonical row, the front UTF-8 text and NUL, the back UTF-8 text and NUL. The remainder of the block is zero. Front blocks are sorted by (folded front, row); back blocks by (folded back, row). Every row appears exactly once per direction. Each pair is two nonblank fields with no TAB/newline/control inside either, at most 191 bytes including the TAB that joins them in a row.

## Append area: fixed 208-byte slots, at most 512

Slot J starts at `base_end + 208*J`, for J=0..511. Base and all earlier physical bytes remain unchanged on append, including incomplete slots.

| Slot offset | Bytes | Meaning |
|---:|---:|---|
| 0 | 4 | ASCII `ADD1`, or `REP2` / `DEL2` (versions 2–4) |
| 4 | 4 | `ADD1`: physical slot number J; mutation: immutable target identity |
| 8 | 192 | canonical `front TAB back NUL`, remaining bytes zero; `DEL2`: all zero |
| 200 | 4 | CRC32 of slot bytes [0,200) |
| 204 | 4 | commit marker ASCII `OK01` |

Append sequence: freshly reopen and scan the bounded addition area; return success without another append when the exact canonical row already exists; check slot capacity; seek physical EOF; pad only the remainder of an incomplete final slot with zeros; write the next slot's first 204 bytes; sync; write four commit bytes; sync; close; reopen and read back all 208 expected bytes; close. Only then report a newly appended record as successful. No base rewrite, rename, unlink, truncation or separate dictionary SAV occurs.

A final short slot, or a complete slot without the exact commit marker, is uncommitted and ignored. It permanently consumes a physical slot. Retry skips it and appends after it, preserving every previous byte. Repeated interruption can exhaust slots before 512 valid pairs; use PC Open .dict / Save As to produce a new compacted indexed base. A full valid committed row remains recognized after an ambiguous returned sync/close/readback error, so retry is idempotent. Duplicate detection applies to exact prior additions (case-sensitive canonical bytes), not existing base entries or different translations.

A version-4 base identity is its canonical row ordinal `0..N-1`, **not its current match position or text**. An added-row identity is `0x80000000 | original_ADD1_slot`. `REP2` replaces that identity's live canonical pair; `DEL2` removes it from both directions. Subsequent mutations retain the original identity, including after key changes. Mutation targets must refer to a base ordinal or an earlier committed ADD1, never a replacement slot or uncommitted hole. The latest valid mutation wins; deletion is terminal (repeated deletion is accepted, replacement after deletion is corruption). Distinct duplicate senses remain distinct. All-deleted live views are valid, although PC output still requires a nonempty compacted base.

All operation types and retired slots share the **512 physical-slot limit**. Repeated edits consume slots too. Exact replacement retries against the latest committed state and repeated deletion retries return success without another write. PC Open .dict / Save As applies the complete log, drops superseded/deleted rows and holes, and rebuilds both block indexes in a new version-4 file. Canonical identities are local to one file and are reassigned on compaction; never retain a selected-row identity across replacing that file. Dictionary edits do not modify copies already stored in learning-list TXT files.

A slot with the exact commit marker but invalid magic/target/sequence/padding/UTF-8/CRC blocks further appends. Lookup fails closed as a whole: it never exposes the immutable base as a fallback that could resurrect a deleted/replaced row. Legacy v1/v2/v3 files are never searched or mutated by the ROM. Invalid records are not silently discarded. Back up the file and filesystem before PC recovery. CRC/commit checks do not promise atomic physical sector writes; real Supercard power-loss behavior is unverified.

## Runtime budgets and evidence

The dictionary screen exclusively borrows the inactive persistent TXT candidate FIL and the three 4 KiB TXT I/O windows; no TXT load/save runs until the catalog is destroyed. The windows become two 4 KiB block buffers and eight 512-byte directory-sector buffers, all tagged by file and offset and invalidated whenever the dictionary file is closed, so no permanent EWRAM is added. A failed close retains its existing quarantine flag for the next borrower. Opened read-only, the dictionary FIL gets a 32-entry FatFS fast-seek cluster map (IWRAM .bss, like the cache tags); a file in more than 15 fragments keeps ordinary seeking.

The GBA validates the header once per view, each directory sector and block by CRC when it is loaded, and each entry's row, lengths, UTF-8 and text grammar when it is used. It does not verify the whole-base CRC or the sorted order of every block; the PC reader verifies both fully. Maliciously resealed blocks with valid CRCs but wrong order are outside accidental-damage detection: queries remain memory-safe but may miss rows.

A prefix lookup is two partitions (first match, first non-match). Each binary-searches the directory, reads at most one block's first entry for an undecidable directory comparison, and then binary-searches inside one block in RAM. An empty query returns the whole base with no reads. When a query extends the previous one in the same direction (valid UTF-8), the search runs inside the previous range. The screen searches after four frames without typing, and any result action searches the current text first. Measured on a real 64,662-entry German–Spanish file with the GBA cache sizes, typing six words letter by letter costs about 3.8 card reads per character including the two visible rows (whole blocks and directory sectors), against roughly 110 small reads with version 3. These are host read counts, not flashcard timings.

Additions use a bounded 512-element 16-bit matching-slot index; query changes scan only the append area (at most 106,496 bytes). A bounded temporary 512-entry canonical-identity table collapses revisions and validates targets while reading only the append log. A separate 512-entry 32-bit sparse rank index suppresses affected base matches: a front rank comes from the record table, a back rank from a (key, row) partition. No query or mutation scans the entire base.

`bash tests/run_dictionary_tests.sh` tests 40,010 synthetic base rows in both directions, damaged headers/blocks/directory sectors/record cells, every read failure position of a query, narrowing hints, canonical selection and additions, every partial append boundary and every single-byte slot mutation, faults and retries, and 24-file discovery. `--sanitize` enables ASan/UBSan for the pure parser and production adapter tests. Neither host measurements nor host-backed API files certify actual SD or FAT sector durability.

Old unpublished dictionary SAV formats are unsupported. They are never automatically deleted, migrated or interpreted as indexed dictionaries. Preserve them for manual recovery. Per-list metadata is separate and documented in `list-metadata-format.md`: matching TXT/SAV basenames, all directly in `/gbavocab`.
