#pragma once
// gbavocab V4.1: build a version-4 .dict on the GBA from a TAB-separated
// UTF-8 export (dict.cc style). The output is byte-identical to the PC
// builder's build_dict() for the same cleaned rows, name, codes and labels.
//
// RAM is far too small for 70,000+ entries, so rows are sorted on the SD card:
// chunks are sorted in RAM and written as runs, then merged (K-way, several
// levels when needed), once for each language. Rows the reader could not use
// are skipped automatically and counted: pairs over 191 bytes, empty or
// missing fields, control characters, invalid UTF-8; exact duplicate pairs
// keep their first copy. Comment, blank, decorative and "Box N" lines are
// ignored like in the PC builder.
#include <cstdint>
#include "ff.h"

struct DictBuildWorkspace {
    unsigned char* ram = nullptr;   // >= 48 KiB, 4-byte aligned
    unsigned ram_size = 0;
    unsigned char* io[3] = {};      // three 4 KiB buffers
};

struct DictBuildMeta {
    char name[32] = {};             // 1..31 UTF-8 bytes
    char front[12] = {}, back[12] = {};
    char front_label[24] = {}, back_label[24] = {};
    bool valid() const;
};

struct DictBuildStats {
    uint32_t imported = 0, too_long = 0, invalid = 0, duplicates = 0;
};

enum class DictBuildError { none, read, write, empty, too_large, meta };
const char* dict_build_error_text(DictBuildError error);

// step 1..4, percent 0..100.
using DictBuildProgress = void (*)(void* context, int step, int percent);

// Language pair from a dict.cc header (first lines of the file). Fills codes,
// labels and name ("German-Spanish") and returns true when found.
bool dict_build_detect(const char* source, DictBuildMeta& meta, const DictBuildWorkspace& work);
// Name from a file name: without ".txt", at most 31 UTF-8 bytes.
void dict_build_name_from_file(const char* filename, char out[32]);
// Label for a known language code ("de" -> "German"), else the code itself.
void dict_build_label(const char* code, char out[24]);

// Builds source into target (absolute paths). Temporary files live in the
// settings folder and are removed; target is only created (by rename) after
// everything was written. target must not exist.
DictBuildError dict_build(const char* source, const char* target, const DictBuildMeta& meta,
                          const DictBuildWorkspace& work, DictBuildStats& stats,
                          DictBuildProgress progress, void* context);

// Tests: smaller chunks and fewer merge inputs force several merge levels.
void dict_build_limits_for_tests(unsigned chunk_bytes, unsigned merge_ways);
