#pragma once
#include "vocab.h"
#include <type_traits>
#include <utility>

// Sources either hand out whole chunks (fill: pointer, length and absolute
// offset of the next bytes) or single characters (next). The chunked path
// finds row ends with memchr and copies whole runs; both paths implement the
// same grammar byte for byte, including offsets, overflow and CR handling.
namespace vocab_scanner_detail {
template<class S, class = void> struct chunked : std::false_type {};
template<class S> struct chunked<S, decltype(void(std::declval<S&>().fill(std::declval<const char*&>(), std::declval<unsigned&>(), std::declval<uint32_t&>())))> : std::true_type {};

template<class Source, bool = chunked<Source>::value> struct Reader;
template<class Source> struct Reader<Source, true> {
    Source& source; const char* data = nullptr; unsigned n = 0, pos = 0; uint32_t base = 0;
    // Append this physical row's bytes to row (up to max), stopping after the
    // LF. Returns false only when no byte at all was available.
    bool row(char* row, int& length, int max, bool& overflow, uint32_t& start, uint32_t& last) {
        bool any = false;
        for (;;) {
            if (pos >= n) { pos = 0; n = 0; if (!source.fill(data, n, base) || !n) return any; }
            if (!any) { start = base + pos; any = true; }
            const char* p = data + pos; unsigned avail = n - pos;
            const char* lf = static_cast<const char*>(std::memchr(p, '\n', avail));
            unsigned take = lf ? unsigned(lf - p) : avail;
            unsigned room = unsigned(max - length);
            if (take > room) { std::memcpy(row + length, p, room); length = max; overflow = true; }
            else { std::memcpy(row + length, p, take); length += int(take); }
            if (lf) { last = base + pos + take; pos += take + 1; return true; }
            last = base + pos + avail - 1; pos = n;
        }
    }
};
template<class Source> struct Reader<Source, false> {
    Source& source;
    bool row(char* row, int& length, int max, bool& overflow, uint32_t& start, uint32_t& last) {
        bool any = false; char ch; uint32_t offset = 0;
        while (source.next(ch, offset)) {
            if (!any) { start = offset; any = true; }
            last = offset;
            if (ch == '\n') break;
            if (length < max) row[length++] = ch;
            else overflow = true;
        }
        return any;
    }
};
}

// One scanner grammar for indexing and persisted-output validation. The visitor
// sees each accepted raw row while it is still in bounded scanner scratch.
// Returning false aborts with -1; I/O failure remains the Source's contract.
template<class Source, class Visitor>
int vocab_scan_visit(Source& source, Visitor&& visit, uint32_t& rejected, PairMetadata* languages = nullptr,
                     uint32_t* footer_start = nullptr, uint32_t* footer_end = nullptr)
{
    rejected = 0;
    int count = 0, box = 1;
    bool footer = false;
    char row[VOCAB_RAW_LINE_MAX + 1];
    vocab_scanner_detail::Reader<Source> reader{source};
    while (true) {
        int length = 0;
        bool overflow = false;
        uint32_t start = 0, offset = 0;
        if (!reader.row(row, length, VOCAB_RAW_LINE_MAX, overflow, start, offset)) break;
        if (length && row[length - 1] == '\r') --length;
        if (overflow || length >= VOCAB_RAW_LINE_MAX) { ++rejected; continue; }
        row[length] = 0;
        if (footer) { if(length) ++rejected; continue; }
        if (PairMetadata::starts(row,"# gbavocab:")) {
            PairMetadata parsed;
            if (std::strlen(row)!=unsigned(length) || !parsed.parse(row)) { ++rejected; continue; }
            if(languages) *languages = parsed;
            if(footer_start) *footer_start=start;
            if(footer_end) *footer_end=offset+1;
            footer = true; continue;
        }
        if (!length) { if (box < 5) ++box; continue; }
        if (!vocab_validate_raw_row(row, length)) { ++rejected; continue; }
        if (count == VOCAB_MAX_LINES) { ++rejected; continue; }
        if (!visit(count, start, box, row, length)) return -1;
        ++count;
    }
    return count;
}
template<class Source>
int vocab_scan(Source& source, VocabFile& vf)
{
    vf.reset();
    vf.line_count = vocab_scan_visit(source,
        [&vf](int i, uint32_t start, int box, const char*, int) {
            vf.line_offsets[i] = start;
            vf.field[i] = uint8_t(box);
            ++vf.field_counts[box - 1];
            return true;
        }, vf.rejected_rows, &vf.languages, &vf.legacy_pair_start, &vf.legacy_pair_end);
    vf.loaded = vf.line_count > 0;
    return vf.line_count;
}
