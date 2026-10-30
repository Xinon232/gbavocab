// On-GBA .dict builder (see dict_build.h). Mirrors builder/dictionary_builder.py
// build_dict(): same fold, same (key, row) order, same greedy 4 KiB block
// packing, directory sectors, record table, header and CRCs.
#include "dict_build.h"
#include "app_config.h"
#include "dictionary_fold.h"
#include "pair_metadata.h"
#include "writer_core.h"
#include <cstring>

namespace {
constexpr unsigned BLOCK = 4096, SECTOR = 512, DIR_ENTRIES = 15, DIR_KEY = 24;
constexpr unsigned REC_MAX = 6 + 191 + 2;          // row, lengths, a NUL b NUL
constexpr uint32_t BASE_LIMIT = 64u * 1024 * 1024, MAX_ENTRIES = (1u << 24) - 1;
constexpr unsigned RUN_LIST = 1024;                 // run boundaries per language
constexpr unsigned DUP_LIMIT = 4096, GROUP_BYTES = 16 * 1024;
unsigned s_chunk_limit = 0, s_ways = 12;

// ---------- small helpers ----------
constexpr uint32_t crc_table[16] = {0x0u,0x1db71064u,0x3b6e20c8u,0x26d930acu,0x76dc4190u,0x6b6b51f4u,0x4db26158u,0x5005713cu,
    0xedb88320u,0xf00f9344u,0xd6d6a3e8u,0xcb61b38cu,0x9b64c2b0u,0x86d3d2d4u,0xa00ae278u,0xbdbdf21cu};
uint32_t crc_more(uint32_t c, const unsigned char* p, unsigned n) {
    while (n--) { c ^= *p++; c = (c >> 4) ^ crc_table[c & 15]; c = (c >> 4) ^ crc_table[c & 15]; }
    return c;
}
uint32_t crc32(const unsigned char* p, unsigned n) { return ~crc_more(~0u, p, n); }
void put32(unsigned char* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = static_cast<unsigned char>(v >> (8 * i)); }
void put16(unsigned char* p, unsigned v) { p[0] = static_cast<unsigned char>(v); p[1] = static_cast<unsigned char>(v >> 8); }
uint32_t get32(const unsigned char* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }

// Record: u32 provisional row, u8 front length, u8 back length, front NUL, back NUL.
unsigned rec_size(const unsigned char* r) { return 6u + r[4] + 1u + r[5] + 1u; }
const char* rec_a(const unsigned char* r) { return reinterpret_cast<const char*>(r + 6); }
const char* rec_b(const unsigned char* r) { return reinterpret_cast<const char*>(r + 6 + r[4] + 1); }
int rec_compare(const unsigned char* x, const unsigned char* y, int side) {
    int c = dictionary_fold::compare(side ? rec_b(x) : rec_a(x), side ? rec_b(y) : rec_a(y));
    if (c) return c;
    uint32_t a = get32(x), b = get32(y);
    return a < b ? -1 : a > b ? 1 : 0;
}

// Folded key bytes (as the reader's Query fold / Python key()): the first 24
// bytes and the full length.
unsigned folded_key(const char* text, unsigned char out[DIR_KEY]) {
    dictionary_fold::Iterator it(text);
    unsigned n = 0;
    for (unsigned c; (c = it.next()) != 0;) {
        unsigned char u[4]; unsigned k = 0;
        if (c >= 0x110000) u[k++] = static_cast<unsigned char>(c - 0x110000);
        else if (c < 0x80) u[k++] = static_cast<unsigned char>(c);
        else if (c < 0x800) { u[k++] = 0xc0 | (c >> 6); u[k++] = 0x80 | (c & 63); }
        else if (c < 0x10000) { u[k++] = 0xe0 | (c >> 12); u[k++] = 0x80 | ((c >> 6) & 63); u[k++] = 0x80 | (c & 63); }
        else { u[k++] = 0xf0 | (c >> 18); u[k++] = 0x80 | ((c >> 12) & 63); u[k++] = 0x80 | ((c >> 6) & 63); u[k++] = 0x80 | (c & 63); }
        for (unsigned i = 0; i < k; ++i, ++n) if (n < DIR_KEY) out[n] = u[i];
    }
    return n;
}

// ---------- temporary files ----------
const char* const temp_tags[] = {"RF0", "RF1", "RB0", "RB1", "FS", "BF", "DF", "BB", "DB", "RW", "OUT"};
enum Temp { RF0, RF1, RB0, RB1, FS, BF, DF, BB, DB, RW, OUT, TEMP_COUNT };
void temp_path(int t, char out[64]) {
    std::strcpy(out, ConfigStore::directory());
    std::strcat(out, "/BLD_");
    std::strcat(out, temp_tags[t]);
    std::strcat(out, ".TMP");
}
void remove_temps() { char p[64]; for (int t = 0; t < TEMP_COUNT; ++t) { temp_path(t, p); f_unlink(p); } }

// Buffered sequential writer (buffer may be null: direct writes).
struct Out {
    FIL f;
    bool open = false, ok = true, crc = false;
    unsigned char* buf = nullptr; unsigned cap = 0, used = 0;
    uint32_t total = 0, c = ~0u;
    bool create(const char* path, unsigned char* b, unsigned n) {
        buf = b; cap = n; used = 0; total = 0; ok = true;
        open = f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK;
        return ok = open;
    }
    bool raw(const void* p, unsigned n) {
        UINT put = 0;
        if (ok && n && (f_write(&f, p, n, &put) != FR_OK || put != n)) ok = false;
        return ok;
    }
    bool write(const void* data, unsigned n) {
        auto p = static_cast<const unsigned char*>(data);
        if (crc) c = crc_more(c, p, n);
        total += n;
        if (!buf) return raw(p, n);
        while (n && ok) {
            unsigned take = cap - used < n ? cap - used : n;
            std::memcpy(buf + used, p, take); used += take; p += take; n -= take;
            if (used == cap) { raw(buf, used); used = 0; }
        }
        return ok;
    }
    bool zeros(unsigned n) { static const unsigned char z[64] = {}; while (n && ok) { unsigned t = n < 64 ? n : 64; write(z, t); n -= t; } return ok; }
    bool flush() { if (buf && used) { raw(buf, used); used = 0; } return ok; }
    bool close() {
        if (!open) return ok;
        flush();
        if (f_sync(&f) != FR_OK) ok = false;
        if (f_close(&f) != FR_OK) ok = false;
        open = false;
        return ok;
    }
    ~Out() { if (open) f_close(&f); }
};

// Buffered sequential reader.
struct In {
    FIL f;
    bool open = false, ok = true;
    unsigned char* buf = nullptr; unsigned cap = 0, pos = 0, len = 0;
    bool start(const char* path, unsigned char* b, unsigned n) {
        buf = b; cap = n; pos = len = 0; ok = true;
        open = f_open(&f, path, FA_READ) == FR_OK;
        return ok = open;
    }
    // Copies n bytes; false at the end or on error.
    bool read(void* data, unsigned n) {
        auto p = static_cast<unsigned char*>(data);
        while (n) {
            if (pos == len) {
                UINT got = 0;
                if (f_read(&f, buf, cap, &got) != FR_OK) { ok = false; return false; }
                if (!got) return false;
                pos = 0; len = got;
            }
            unsigned take = len - pos < n ? len - pos : n;
            std::memcpy(p, buf + pos, take); pos += take; p += take; n -= take;
        }
        return true;
    }
    bool record(unsigned char* r) { return read(r, 6) && read(r + 6, r[4] + 1u + r[5] + 1u); }
    // Next byte, or -1 at the end / on error.
    int get() {
        if (pos == len) {
            UINT got = 0;
            if (f_read(&f, buf, cap, &got) != FR_OK) { ok = false; return -1; }
            if (!got) return -1;
            pos = 0; len = got;
        }
        return buf[pos++];
    }
    uint32_t size() { return open ? uint32_t(f_size(&f)) : 0; }
    ~In() { if (open) f_close(&f); }
};

// ---------- progress ----------
struct Progress {
    DictBuildProgress fn; void* context; int step = 0, shown = -1;
    void set(int s, uint64_t done, uint64_t total) {
        int percent = total ? int(done * 100 / total) : 100;
        if (percent > 100) percent = 100;
        if (s != step || percent != shown) { step = s; shown = percent; if (fn) fn(context, s, percent); }
    }
};

// ---------- 4 KiB block packing (Python _pack_blocks / _block_bytes) ----------
struct Packer {
    int side = 0;
    Out* blocks = nullptr; Out* dir = nullptr; Out* rows = nullptr;
    unsigned char* stage = nullptr;   // entries of the open block, BLOCK bytes
    unsigned char* block = nullptr;   // assembled block, BLOCK bytes
    uint16_t* lens = nullptr;         // entry sizes, BLOCK / 7 entries
    unsigned used = 8, count = 0, staged = 0;
    uint32_t rank = 0, total = 0;
    unsigned char key[DIR_KEY]; unsigned key_n = 0;
    bool flush() {
        if (!count) return true;
        std::memset(block, 0, BLOCK);
        put16(block + 4, count);
        unsigned at = 8 + 2 * count, from = 0;
        for (unsigned i = 0; i < count; ++i) {
            put16(block + 8 + 2 * i, at);
            std::memcpy(block + at, stage + from, lens[i]);
            at += lens[i]; from += lens[i];
        }
        put32(block, crc32(block + 4, BLOCK - 4));
        unsigned char d[32] = {};
        put32(d, rank); put16(d + 4, count);
        d[6] = static_cast<unsigned char>(key_n < DIR_KEY ? key_n : DIR_KEY);
        d[7] = key_n > DIR_KEY ? 1 : 0;
        std::memcpy(d + 8, key, d[6]);
        bool ok = blocks->write(block, BLOCK) && dir->write(d, 32);
        rank += count; count = 0; used = 8; staged = 0;
        return ok;
    }
    bool add(uint32_t row, const unsigned char* r) {
        unsigned an = r[4], bn = r[5], size = 3 + an + 1 + bn + 1;
        if (count && used + 2 + size > BLOCK && !flush()) return false;
        if (!count) key_n = folded_key(side ? rec_b(r) : rec_a(r), key);
        unsigned char* e = stage + staged;
        e[0] = static_cast<unsigned char>(row); e[1] = static_cast<unsigned char>(row >> 8); e[2] = static_cast<unsigned char>(row >> 16);
        std::memcpy(e + 3, rec_a(r), an + 1u);
        std::memcpy(e + 3 + an + 1, rec_b(r), bn + 1u);
        lens[count++] = static_cast<uint16_t>(size); staged += size; used += 2 + size; ++total;
        if (rows) { unsigned char v[4]; put32(v, row); return rows->write(v, 4); }
        return true;
    }
};

// Dropped duplicate rows (sorted after the first language) and renumbering.
struct Drops {
    uint32_t* rows = nullptr; unsigned count = 0;
    void sort() {   // shell sort: at most DUP_LIMIT values
        for (unsigned gap = count / 2; gap; gap /= 2)
            for (unsigned i = gap; i < count; ++i)
                for (unsigned j = i; j >= gap && rows[j - gap] > rows[j]; j -= gap) { uint32_t t = rows[j]; rows[j] = rows[j - gap]; rows[j - gap] = t; }
    }
    unsigned below(uint32_t row) const { unsigned lo = 0, hi = count; while (lo < hi) { unsigned m = (lo + hi) / 2; if (rows[m] < row) lo = m + 1; else hi = m; } return lo; }
    bool contains(uint32_t row) const { unsigned i = below(row); return i < count && rows[i] == row; }
};

// ---------- sinks for the final merge of each language ----------
struct Sink { virtual bool put(const unsigned char* r) = 0; protected: ~Sink() = default; };
struct RunSink final : Sink { Out* out; bool put(const unsigned char* r) override { return out->write(r, rec_size(r)); } };
// First language: drop exact duplicate pairs (first copy kept). They share the
// folded front key, so only the current equal-key group is compared.
struct DedupSink final : Sink {
    // The group's kept records, packed (GROUP_BYTES: a few hundred entries).
    Out* out = nullptr; Drops* drops = nullptr; unsigned char* group = nullptr; unsigned used = 0; DictBuildStats* stats = nullptr;
    bool put(const unsigned char* r) override {
        const unsigned size = rec_size(r);
        if (used && dictionary_fold::compare(rec_a(group), rec_a(r))) used = 0;
        for (unsigned at = 0; at < used; at += rec_size(group + at)) {
            const unsigned char* g = group + at;
            if (g[4] == r[4] && g[5] == r[5] && !std::memcmp(g + 6, r + 6, size - 6) && drops->count < DUP_LIMIT) {
                drops->rows[drops->count++] = get32(r);
                ++stats->duplicates;
                return true;
            }
        }
        if (used + size <= GROUP_BYTES) { std::memcpy(group + used, r, size); used += size; }
        return out->write(r, size);
    }
};
// Second language: leave out the dropped rows, renumber, pack.
struct PackSink final : Sink {
    Packer* packer = nullptr; const Drops* drops = nullptr;
    bool put(const unsigned char* r) override {
        uint32_t row = get32(r);
        if (drops->contains(row)) return true;
        return packer->add(row - drops->below(row), r);
    }
};

// ---------- K-way merge of runs stored back to back in one file ----------
struct Source { FIL f; uint32_t left; bool open; unsigned char rec[REC_MAX]; };
bool load(Source& s, bool& error) {
    if (!s.left) return false;
    UINT got = 0;
    if (f_read(&s.f, s.rec, 6, &got) != FR_OK || got != 6) { error = true; return false; }
    unsigned rest = s.rec[4] + 1u + s.rec[5] + 1u;
    if (6 + rest > s.left || f_read(&s.f, s.rec + 6, rest, &got) != FR_OK || got != rest) { error = true; return false; }
    s.left -= 6 + rest;
    return true;
}

__attribute__((noinline)) bool merge_group(const char* path, const uint32_t* bounds, unsigned count, int side, Sink& sink,
                                           unsigned char* mem, Progress& progress, int step, uint64_t& done, uint64_t total) {
    Source* src = reinterpret_cast<Source*>(mem);
    uint8_t heap[64]; unsigned n = 0;
    bool ok = true, error = false;
    for (unsigned i = 0; i < count; ++i) src[i].open = false;
    auto less = [&](unsigned x, unsigned y) { return rec_compare(src[x].rec, src[y].rec, side) < 0; };
    auto down = [&](unsigned i) {
        for (;;) {
            unsigned l = 2 * i + 1, r = l + 1, m = i;
            if (l < n && less(heap[l], heap[m])) m = l;
            if (r < n && less(heap[r], heap[m])) m = r;
            if (m == i) return;
            uint8_t t = heap[i]; heap[i] = heap[m]; heap[m] = t; i = m;
        }
    };
    for (unsigned i = 0; i < count && ok; ++i) {
        Source& s = src[i];
        if (f_open(&s.f, path, FA_READ) != FR_OK) { ok = false; break; }
        s.open = true;
        if (f_lseek(&s.f, bounds[i]) != FR_OK) { ok = false; break; }
        s.left = bounds[i + 1] - bounds[i];
        if (load(s, error)) heap[n++] = static_cast<uint8_t>(i);
        if (error) ok = false;
    }
    for (unsigned i = n / 2; i-- > 0;) down(i);
    while (ok && n) {
        Source& s = src[heap[0]];
        if (!sink.put(s.rec)) { ok = false; break; }
        progress.set(step, ++done, total);
        if (!load(s, error)) { if (error) { ok = false; break; } heap[0] = heap[--n]; }
        down(0);
    }
    for (unsigned i = 0; i < count; ++i) if (src[i].open && f_close(&src[i].f) != FR_OK) ok = false;
    return ok;
}

// Merges all runs of one language into the final sink, through as many
// intermediate levels (alternating between two files) as needed.
__attribute__((noinline)) DictBuildError merge_all(int side, int a, int b, uint32_t* bounds, unsigned runs, Sink& final_sink,
                                                   unsigned char* mem, unsigned char* io, uint32_t records,
                                                   Progress& progress, int step) {
    unsigned levels = 1;
    for (unsigned r = runs; r > s_ways; r = (r + s_ways - 1) / s_ways) ++levels;
    uint64_t done = 0, total = uint64_t(records) * levels;
    char from[64], to[64];
    while (runs > s_ways) {
        temp_path(a, from); temp_path(b, to);
        Out out;
        if (!out.create(to, io, BLOCK)) return DictBuildError::write;
        RunSink sink; sink.out = &out;
        unsigned groups = 0;
        for (unsigned g = 0; g < runs; g += s_ways, ++groups) {
            unsigned n = runs - g < s_ways ? runs - g : s_ways;
            uint32_t local[65];
            for (unsigned i = 0; i <= n; ++i) local[i] = bounds[g + i];
            if (!merge_group(from, local, n, side, sink, mem, progress, step, done, total)) return out.ok ? DictBuildError::read : DictBuildError::write;
            if (!out.flush()) return DictBuildError::write;
            bounds[groups + 1] = out.total;
        }
        if (!out.close()) return DictBuildError::write;
        f_unlink(from);
        bounds[0] = 0; runs = groups;
        int t = a; a = b; b = t;
    }
    temp_path(a, from);
    if (!merge_group(from, bounds, runs, side, final_sink, mem, progress, step, done, total)) return DictBuildError::write;
    f_unlink(from);
    return DictBuildError::none;
}

// ---------- step 1: read, check and sort chunks ----------
bool decorative_or_blank(const unsigned char* s, unsigned n) {
    for (unsigned i = 0; i < n;) {
        unsigned c = s[i];
        if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') { ++i; continue; }
        if (std::strchr("=+-_|", int(c)) && c) { ++i; continue; }
        // ─━│┌┐└┘┼╔╗╚╝═║ (U+2500..U+257F box drawing)
        if (c == 0xe2 && i + 2 < n && s[i + 1] == 0x94) { ++i; i += 2; continue; }
        if (c == 0xe2 && i + 2 < n && s[i + 1] == 0x95) { ++i; i += 2; continue; }
        return false;
    }
    return true;
}
bool box_line(const unsigned char* s, unsigned n) {
    auto filler = [](unsigned c) { return c == ' ' || c == '\v' || c == '\f' || c == '\r' || std::strchr("=[]_*+-", int(c)) != nullptr; };
    unsigned i = 0;
    while (i < n && s[i] && filler(s[i])) ++i;
    auto word = [&](const char* w) {
        unsigned k = 0;
        for (; w[k]; ++k) { if (i + k >= n) return false; unsigned c = s[i + k]; if (c >= 'A' && c <= 'Z') c += 32; if (c != unsigned(w[k])) return false; }
        i += k; return true;
    };
    if (!word("box") && !word("field")) return false;
    while (i < n && (s[i] == ' ' || s[i] == '\v' || s[i] == '\f' || s[i] == '\r')) ++i;
    if (i >= n || s[i] < '1' || s[i] > '5') return false;
    ++i;
    while (i < n && s[i] && filler(s[i])) ++i;
    return i == n;
}
bool field_ok(const unsigned char* p, unsigned n) {
    bool visible = false;
    for (unsigned i = 0; i < n; ++i) { if (p[i] < 32 || p[i] == 127) return false; if (p[i] > 32) visible = true; }
    return visible;
}

// Bottom-up merge sort of record offsets.
void sort_chunk(const unsigned char* recs, uint32_t* p, uint32_t* aux, unsigned n, int side) {
    for (unsigned width = 1; width < n; width *= 2) {
        for (unsigned lo = 0; lo < n; lo += 2 * width) {
            unsigned mid = lo + width < n ? lo + width : n, hi = lo + 2 * width < n ? lo + 2 * width : n;
            unsigned i = lo, j = mid, k = lo;
            while (i < mid && j < hi) aux[k++] = rec_compare(recs + p[j], recs + p[i], side) < 0 ? p[j++] : p[i++];
            while (i < mid) aux[k++] = p[i++];
            while (j < hi) aux[k++] = p[j++];
        }
        std::memcpy(p, aux, n * sizeof(uint32_t));
    }
}

struct Chunk {
    unsigned char* recs; unsigned cap, used = 0;
    uint32_t* p; uint32_t* aux; unsigned max, n = 0;
};

bool write_runs(Chunk& c, Out& f, Out& b, uint32_t* runs_f, uint32_t* runs_b, unsigned& runs) {
    if (!c.n) return true;
    if (runs + 1 >= RUN_LIST) return false;
    Out* outs[2] = {&f, &b};
    uint32_t* lists[2] = {runs_f, runs_b};
    for (int side = 0; side < 2; ++side) {
        sort_chunk(c.recs, c.p, c.aux, c.n, side);
        for (unsigned i = 0; i < c.n; ++i) { const unsigned char* r = c.recs + c.p[i]; if (!outs[side]->write(r, rec_size(r))) return false; }
        lists[side][runs + 1] = outs[side]->total;
    }
    ++runs; c.used = c.n = 0;
    return true;
}

__attribute__((noinline)) DictBuildError read_source(const char* source, const DictBuildWorkspace& w, uint32_t* runs_f, uint32_t* runs_b,
                                                     unsigned& runs, uint32_t& records, DictBuildStats& stats, Progress& progress) {
    In in;
    if (!in.start(source, w.io[0], BLOCK)) return DictBuildError::read;
    uint32_t size = in.size(), consumed = 0;
    char path_f[64], path_b[64];
    temp_path(RF0, path_f); temp_path(RB0, path_b);
    Out f, b;
    if (!f.create(path_f, w.io[1], BLOCK) || !b.create(path_b, w.io[2], BLOCK)) return DictBuildError::write;
    runs_f[0] = runs_b[0] = 0; runs = 0; records = 0;
    // Phase area after the two run lists: line, then records and offsets.
    unsigned char* area = w.ram + 2 * RUN_LIST * 4;
    unsigned area_size = w.ram_size - 2 * RUN_LIST * 4;
    unsigned char* line = area; area += 512; area_size -= 512;
    Chunk c;
    unsigned record_bytes = area_size * 3 / 4;
    if (s_chunk_limit && s_chunk_limit < record_bytes) record_bytes = s_chunk_limit;
    c.recs = area; c.cap = record_bytes;
    c.max = (area_size - record_bytes) / 8;
    c.p = reinterpret_cast<uint32_t*>(area + record_bytes);
    c.aux = c.p + c.max;
    bool first = true;
    for (unsigned lines = 0;; ++lines) {
        // One line: at most 511 bytes kept; longer lines are only measured.
        unsigned n = 0; bool long_line = false, got = false;
        for (int ch; (ch = in.get()) >= 0;) {
            got = true; ++consumed;
            if (ch == '\n') break;
            if (n < 511) line[n++] = static_cast<unsigned char>(ch); else long_line = true;
        }
        if (!in.ok) return DictBuildError::read;
        if (!got) break;
        if (!(lines & 63)) progress.set(1, consumed, size);
        if (n && line[n - 1] == '\r' && !long_line) --n;
        unsigned start = 0;
        if (first && n >= 3 && line[0] == 0xef && line[1] == 0xbb && line[2] == 0xbf) start = 3;
        first = false;
        const unsigned char* s = line + start; n -= start;
        unsigned lead = 0;
        while (lead < n && (s[lead] == ' ' || s[lead] == '\t' || s[lead] == '\v' || s[lead] == '\f' || s[lead] == '\r')) ++lead;
        if (lead < n && s[lead] == '#') continue;
        if (!long_line && decorative_or_blank(s, n)) continue;
        const unsigned char* tab = static_cast<const unsigned char*>(std::memchr(s, '\t', n));
        if (!tab) { if (!long_line && box_line(s, n)) continue; ++stats.invalid; continue; }
        unsigned an = unsigned(tab - s);
        const unsigned char* bstart = tab + 1;
        const unsigned char* tab2 = static_cast<const unsigned char*>(std::memchr(bstart, '\t', n - an - 1));
        unsigned bn = tab2 ? unsigned(tab2 - bstart) : n - an - 1;
        bool cut = long_line && !tab2;   // the back field may continue past what was kept
        if (!field_ok(s, an) || (!cut && !field_ok(bstart, bn))) { ++stats.invalid; continue; }
        if (cut || an + 1 + bn > 191) { ++stats.too_long; continue; }
        if (!writer::valid_utf8(reinterpret_cast<const char*>(s), an) || !writer::valid_utf8(reinterpret_cast<const char*>(bstart), bn)) { ++stats.invalid; continue; }
        if (records >= MAX_ENTRIES) return DictBuildError::too_large;
        unsigned size_r = 6 + an + 1 + bn + 1;
        if (c.used + size_r > c.cap || c.n == c.max) {
            if (!write_runs(c, f, b, runs_f, runs_b, runs)) return f.ok && b.ok ? DictBuildError::too_large : DictBuildError::write;
        }
        unsigned char* r = c.recs + c.used;
        put32(r, records); r[4] = static_cast<unsigned char>(an); r[5] = static_cast<unsigned char>(bn);
        std::memcpy(r + 6, s, an); r[6 + an] = 0;
        std::memcpy(r + 7 + an, bstart, bn); r[7 + an + bn] = 0;
        c.p[c.n++] = c.used; c.used += size_r;
        ++records;
    }
    if (!write_runs(c, f, b, runs_f, runs_b, runs)) return f.ok && b.ok ? DictBuildError::too_large : DictBuildError::write;
    if (!f.close() || !b.close()) return DictBuildError::write;
    progress.set(1, size, size);
    return records ? DictBuildError::none : DictBuildError::empty;
}

// ---------- step 4: the final file ----------
uint32_t align(uint32_t v, uint32_t a) { return (v + a - 1) / a * a; }

__attribute__((noinline)) bool copy_into(Out& out, int temp, unsigned char* io, Progress& progress, uint32_t end) {
    char path[64]; temp_path(temp, path);
    In in;
    if (!in.start(path, io, BLOCK)) return false;
    unsigned char* block = io;   // whole blocks: read straight through the buffer
    for (;;) {
        UINT got = 0;
        if (f_read(&in.f, block, BLOCK, &got) != FR_OK) return false;
        if (!got) break;
        if (!out.write(block, got)) return false;
        progress.set(4, out.total, end);
    }
    return true;
}

__attribute__((noinline)) bool directory_into(Out& out, int temp, unsigned blocks, unsigned char* io, unsigned char* sector) {
    char path[64]; temp_path(temp, path);
    In in;
    if (!in.start(path, io, BLOCK)) return false;
    for (unsigned b = 0; b < blocks; b += DIR_ENTRIES) {
        std::memset(sector, 0, SECTOR);
        unsigned n = blocks - b < DIR_ENTRIES ? blocks - b : DIR_ENTRIES;
        if (!in.read(sector + 4, n * 32)) return false;
        put32(sector, crc32(sector + 4, SECTOR - 4));
        if (!out.write(sector, SECTOR)) return false;
    }
    return true;
}

__attribute__((noinline)) DictBuildError assemble(const DictBuildMeta& meta, uint32_t n, unsigned b0, unsigned b1,
                                                  const DictBuildWorkspace& w, Progress& progress) {
    const uint32_t records = SECTOR, dir0 = align(records + 4 * n, SECTOR);
    const uint32_t dir1 = dir0 + (b0 + DIR_ENTRIES - 1) / DIR_ENTRIES * SECTOR;
    const uint32_t blocks0 = align(dir1 + (b1 + DIR_ENTRIES - 1) / DIR_ENTRIES * SECTOR, BLOCK);
    const uint64_t end64 = uint64_t(blocks0) + uint64_t(b0 + b1) * BLOCK;
    if (end64 > BASE_LIMIT) return DictBuildError::too_large;
    const uint32_t end = uint32_t(end64), blocks1 = blocks0 + b0 * BLOCK;
    char path[64]; temp_path(OUT, path);
    Out out;
    if (!out.create(path, w.io[1], BLOCK)) return DictBuildError::write;
    out.zeros(SECTOR);
    out.crc = true; out.c = ~0u;
    // Record table: row -> front rank, filled window by window from the
    // front-order row list.
    uint32_t* window = reinterpret_cast<uint32_t*>(w.ram);
    const uint32_t per = w.ram_size / 4;
    char rows_path[64]; temp_path(RW, rows_path);
    for (uint32_t base = 0; base < n; base += per) {
        uint32_t count = n - base < per ? n - base : per;
        std::memset(window, 0, count * 4);
        In rows;
        if (!rows.start(rows_path, w.io[0], BLOCK)) return DictBuildError::read;
        for (uint32_t rank = 0; rank < n; ++rank) {
            unsigned char v[4];
            if (!rows.read(v, 4)) return DictBuildError::read;
            uint32_t row = get32(v);
            if (row >= base && row - base < count) put32(reinterpret_cast<unsigned char*>(window + (row - base)), rank);
        }
        if (!out.write(window, count * 4)) return DictBuildError::write;
        progress.set(4, out.total, end);
    }
    out.zeros(dir0 - out.total);
    unsigned char* sector = w.ram;   // the window is no longer needed
    if (!directory_into(out, DF, b0, w.io[0], sector) || !directory_into(out, DB, b1, w.io[0], sector)) return DictBuildError::read;
    out.zeros(blocks0 - out.total);
    if (out.total != blocks0) return DictBuildError::write;
    if (!copy_into(out, BF, w.io[0], progress, end) || out.total != blocks1 || !copy_into(out, BB, w.io[0], progress, end)) return DictBuildError::read;
    if (!out.flush() || out.total != end) return DictBuildError::write;
    // Header.
    unsigned char h[SECTOR] = {};
    std::memcpy(h, "GVDIDX04", 8);
    put32(h + 8, end); put32(h + 12, n);
    std::strcpy(reinterpret_cast<char*>(h + 16), meta.name);
    std::strcpy(reinterpret_cast<char*>(h + 48), meta.front);
    std::strcpy(reinterpret_cast<char*>(h + 60), meta.back);
    std::strcpy(reinterpret_cast<char*>(h + 72), meta.front_label);
    std::strcpy(reinterpret_cast<char*>(h + 96), meta.back_label);
    put32(h + 120, BLOCK); put32(h + 124, records); put32(h + 128, dir0); put32(h + 132, blocks0);
    put32(h + 136, b0); put32(h + 140, dir1); put32(h + 144, blocks1); put32(h + 148, b1);
    put32(h + 152, ~out.c);
    put32(h + 156, crc32(h, 156));
    if (f_lseek(&out.f, 0) != FR_OK || !out.raw(h, SECTOR) || !out.close()) return DictBuildError::write;
    progress.set(4, end, end);
    return DictBuildError::none;
}

bool text_ok(const char* s, unsigned limit) {
    unsigned n = unsigned(std::strlen(s));
    if (!n || n >= limit || !writer::valid_utf8(s, n)) return false;
    for (unsigned i = 0; i < n; ++i) if (static_cast<unsigned char>(s[i]) < 32 || s[i] == 127) return false;
    return true;
}

struct Language { const char* code; const char* label; };
constexpr Language languages[] = {
    {"bg","Bulgarian"},{"bs","Bosnian"},{"cs","Czech"},{"da","Danish"},{"de","German"},{"el","Greek"},{"en","English"},
    {"eo","Esperanto"},{"es","Spanish"},{"fi","Finnish"},{"fr","French"},{"hr","Croatian"},{"hu","Hungarian"},{"is","Icelandic"},
    {"it","Italian"},{"la","Latin"},{"nl","Dutch"},{"no","Norwegian"},{"pl","Polish"},{"pt","Portuguese"},{"ro","Romanian"},
    {"ru","Russian"},{"sk","Slovak"},{"sq","Albanian"},{"sr","Serbian"},{"sv","Swedish"},{"tr","Turkish"},{"uk","Ukrainian"}};
}

bool DictBuildMeta::valid() const {
    return text_ok(name, 32) && PairMetadata::code(front) && PairMetadata::code(back) && std::strcmp(front, back) &&
        text_ok(front_label, 24) && text_ok(back_label, 24);
}

const char* dict_build_error_text(DictBuildError e) {
    switch (e) {
    case DictBuildError::none: return "Dictionary created";
    case DictBuildError::read: return "Cannot read the card";
    case DictBuildError::write: return "Cannot write (card full?)";
    case DictBuildError::empty: return "No usable entries";
    case DictBuildError::too_large: return "Too large for a .dict";
    default: return "Invalid name or languages";
    }
}

void dict_build_label(const char* code, char out[24]) {
    for (const auto& l : languages) if (!std::strcmp(l.code, code)) { std::strcpy(out, l.label); return; }
    std::strncpy(out, code, 23); out[23] = 0;
}

void dict_build_name_from_file(const char* filename, char out[32]) {
    unsigned n = unsigned(std::strlen(filename));
    if (n > 4 && filename[n - 4] == '.') n -= 4;
    if (n > 31) { n = 31; while (n && (static_cast<unsigned char>(filename[n]) & 0xc0) == 0x80) --n; }
    std::memcpy(out, filename, n); out[n] = 0;
    for (unsigned i = 0; i < n; ++i) if (static_cast<unsigned char>(out[i]) < 32 || out[i] == 127) out[i] = '_';
    if (!n) std::strcpy(out, "Dictionary");
}

bool dict_build_detect(const char* source, DictBuildMeta& meta, const DictBuildWorkspace& w) {
    FIL f;
    if (f_open(&f, source, FA_READ) != FR_OK) return false;
    UINT got = 0;
    bool ok = f_read(&f, w.io[0], BLOCK - 1, &got) == FR_OK;
    f_close(&f);
    if (!ok) return false;
    char* text = reinterpret_cast<char*>(w.io[0]);
    text[got] = 0;
    // Only the comment lines at the top: one names dict.cc, one the pair.
    bool dictcc = false; char codes[2][4] = {};
    for (char* line = text; *line;) {
        char* next = std::strchr(line, '\n');
        if (next) *next = 0;
        char* p = line;
        if (p == text && static_cast<unsigned char>(p[0]) == 0xef) p += 3;
        while (*p == ' ' || *p == '\t') ++p;
        if (*p != '#') { if (*p && *p != '\r') break; }
        else {
            for (char* q = p; *q; ++q) {
                if ((q[0] == 'd' || q[0] == 'D') && !std::strncmp(q + 1, "ict.cc", 6)) dictcc = true;
                auto letter = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };
                if (!codes[0][0] && letter(q[0]) && (q == p || !letter(q[-1]))) {
                    unsigned a = 0; while (letter(q[a]) && a < 4) ++a;
                    if (a >= 2 && a <= 3 && q[a] == '-') {
                        unsigned b = 0; while (letter(q[a + 1 + b]) && b < 4) ++b;
                        if (b >= 2 && b <= 3 && !letter(q[a + 1 + b])) {
                            bool upper = true;
                            for (unsigned i = 0; i < a; ++i) if (q[i] < 'A' || q[i] > 'Z') upper = false;
                            for (unsigned i = 0; i < b; ++i) if (q[a + 1 + i] < 'A' || q[a + 1 + i] > 'Z') upper = false;
                            if (upper) {
                                for (unsigned i = 0; i < a; ++i) codes[0][i] = char(q[i] + 32);
                                for (unsigned i = 0; i < b; ++i) codes[1][i] = char(q[a + 1 + i] + 32);
                            }
                        }
                    }
                }
            }
        }
        if (!next) break;
        line = next + 1;
    }
    if (!dictcc || !codes[0][0] || !std::strcmp(codes[0], codes[1])) return false;
    std::strcpy(meta.front, codes[0]); std::strcpy(meta.back, codes[1]);
    dict_build_label(meta.front, meta.front_label); dict_build_label(meta.back, meta.back_label);
    char name[64]; std::strcpy(name, meta.front_label); std::strcat(name, "-"); std::strcat(name, meta.back_label);
    if (std::strlen(name) < 32) std::strcpy(meta.name, name);
    return true;
}

void dict_build_limits_for_tests(unsigned chunk_bytes, unsigned merge_ways) {
    s_chunk_limit = chunk_bytes;
    s_ways = merge_ways < 2 ? 2 : merge_ways > 64 ? 64 : merge_ways;
}

DictBuildError dict_build(const char* source, const char* target, const DictBuildMeta& meta,
                          const DictBuildWorkspace& w, DictBuildStats& stats,
                          DictBuildProgress progress_fn, void* context) {
    stats = DictBuildStats();
    if (!meta.valid()) return DictBuildError::meta;
    Progress progress{progress_fn, context};
    FRESULT made = f_mkdir("/gbavocab");
    (void)made;
    made = f_mkdir(ConfigStore::directory());
    if (made != FR_OK && made != FR_EXIST) return DictBuildError::write;
    remove_temps();
    auto finish = [&](DictBuildError e) { remove_temps(); return e; };

    uint32_t* runs_f = reinterpret_cast<uint32_t*>(w.ram);
    uint32_t* runs_b = runs_f + RUN_LIST;
    unsigned runs = 0; uint32_t records = 0;
    DictBuildError e = read_source(source, w, runs_f, runs_b, runs, records, stats, progress);
    if (e != DictBuildError::none) return finish(e);

    // RAM after the run lists: drops, duplicate group, merge sources, packer.
    unsigned char* area = w.ram + 2 * RUN_LIST * 4;
    Drops drops; drops.rows = reinterpret_cast<uint32_t*>(area); area += DUP_LIMIT * 4;
    unsigned char* group = area; area += GROUP_BYTES;
    unsigned char* stage = area; area += BLOCK;
    uint16_t* lens = reinterpret_cast<uint16_t*>(area); area += BLOCK / 7 * 2 + 4;
    unsigned char* small = area; area += 512;           // directory entries buffer
    unsigned char* rows_buf = area; area += BLOCK;       // front-order rows buffer
    area = reinterpret_cast<unsigned char*>((reinterpret_cast<uintptr_t>(area) + 7) & ~uintptr_t(7));
    unsigned char* merge_mem = area;
    if (merge_mem + s_ways * sizeof(Source) > w.ram + w.ram_size) return finish(DictBuildError::too_large);

    // Step 2: first language order (duplicates dropped) into FS.
    {
        char fs_path[64]; temp_path(FS, fs_path);
        Out fs;
        if (!fs.create(fs_path, rows_buf, BLOCK)) return finish(DictBuildError::write);
        DedupSink sink; sink.out = &fs; sink.drops = &drops; sink.group = group; sink.stats = &stats;
        e = merge_all(0, RF0, RF1, runs_f, runs, sink, merge_mem, w.io[1], records, progress, 2);
        if (e != DictBuildError::none) return finish(e);
        if (!fs.close()) return finish(DictBuildError::write);
    }
    drops.sort();
    const uint32_t n = records - drops.count;
    unsigned b0 = 0, b1 = 0;
    // Front blocks from FS, renumbered.
    {
        char p[64];
        Out blocks, dir, rows;
        temp_path(BF, p); if (!blocks.create(p, nullptr, 0)) return finish(DictBuildError::write);
        temp_path(DF, p); if (!dir.create(p, small, 512)) return finish(DictBuildError::write);
        temp_path(RW, p); if (!rows.create(p, rows_buf, BLOCK)) return finish(DictBuildError::write);
        Packer pack; pack.side = 0; pack.blocks = &blocks; pack.dir = &dir; pack.rows = &rows;
        pack.stage = stage; pack.block = w.io[2]; pack.lens = lens;
        temp_path(FS, p);
        In in;
        if (!in.start(p, w.io[0], BLOCK)) return finish(DictBuildError::read);
        unsigned char r[REC_MAX];
        while (in.record(r)) { uint32_t row = get32(r); if (!pack.add(row - drops.below(row), r)) return finish(DictBuildError::write); }
        if (!in.ok) return finish(DictBuildError::read);
        if (!pack.flush() || !blocks.close() || !dir.close() || !rows.close()) return finish(DictBuildError::write);
        if (pack.total != n) return finish(DictBuildError::read);
        b0 = unsigned(dir.total / 32);
    }
    // Step 3: second language order straight into its blocks.
    {
        char p[64];
        Out blocks, dir;
        temp_path(BB, p); if (!blocks.create(p, nullptr, 0)) return finish(DictBuildError::write);
        temp_path(DB, p); if (!dir.create(p, small, 512)) return finish(DictBuildError::write);
        Packer pack; pack.side = 1; pack.blocks = &blocks; pack.dir = &dir;
        pack.stage = stage; pack.block = w.io[2]; pack.lens = lens;
        PackSink sink; sink.packer = &pack; sink.drops = &drops;
        e = merge_all(1, RB0, RB1, runs_b, runs, sink, merge_mem, w.io[1], records, progress, 3);
        if (e != DictBuildError::none) return finish(e);
        if (!pack.flush() || !blocks.close() || !dir.close()) return finish(DictBuildError::write);
        if (pack.total != n) return finish(DictBuildError::read);
        b1 = unsigned(dir.total / 32);
    }
    // Step 4: header, record table, directories and blocks; then the name.
    e = assemble(meta, n, b0, b1, w, progress);
    if (e != DictBuildError::none) return finish(e);
    char out_path[64]; temp_path(OUT, out_path);
    if (f_rename(out_path, target) != FR_OK) return finish(DictBuildError::write);
    stats.imported = n;
    return finish(DictBuildError::none);
}
