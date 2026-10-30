#include "app_config.h"
#include "exfat_repair.h"
#include <cstring>

using namespace app_config;

namespace {
#if defined(__DEVKITARM__) || defined(VOCAB_ROOT_DIRECTORY)
#define CONFIG_DIR "/gbavocab/GBAVOCAB.SYS"
#else
#define CONFIG_DIR "/GBAVOCAB.SYS"
#endif
const char* const slot_names[2] = {CONFIG_DIR "/CONFIG0.DAT", CONFIG_DIR "/CONFIG1.DAT"};
// Generation of the newest valid slot and which slot that is; a save goes to
// the other one. Before any read, slot 1 counts as newest (first save: slot 0).
uint32_t s_generation = 0;
int s_newest = 1;

uint32_t get32(const unsigned char* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
void put32(unsigned char* p, uint32_t v) {
    for (int i = 0; i < 4; ++i) p[i] = static_cast<unsigned char>(v >> (8 * i));
}
uint32_t fnv(const unsigned char* p, unsigned n) {
    uint32_t h = 2166136261u;
    while (n--) h = (h ^ *p++) * 16777619u;
    return h;
}
// FAT names are case-insensitive.
bool same_name(const char* a, const char* b) {
    for (;; ++a, ++b) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x += 32;
        if (y >= 'A' && y <= 'Z') y += 32;
        if (x != y) return false;
        if (!x) return true;
    }
}
bool terminated(const unsigned char* p, unsigned n) {
    for (unsigned i = 0; i < n; ++i) if (!p[i]) return true;
    return false;
}
bool name_ok(const char* list) {
    if (!list || !list[0]) return false;
    unsigned n = 0;
    while (list[n]) if (++n >= 64) return false;
    return !std::strchr(list, '/') && !std::strchr(list, '\\') && !std::strchr(list, ':');
}
bool record_ok(const unsigned char* r) {
    if (!r[0] || !terminated(r, 64) || !terminated(r + 64, 12) || !terminated(r + 76, 12) ||
        !terminated(r + 88, 64)) return false;
    const char* front = reinterpret_cast<const char*>(r + 64);
    const char* back = reinterpret_cast<const char*>(r + 76);
    PairMetadata pair;
    if ((front[0] || back[0]) && !pair.set(front, back)) return false;
    return r[152] >= 1 && r[152] <= 3 && ListSettings::filename(reinterpret_cast<const char*>(r + 88));
}
}

const char* ConfigStore::directory() { return CONFIG_DIR; }

// exFAT: FR_INT_ERR from a half-written entry set (power cut) in the root,
// /gbavocab or GBAVOCAB.SYS: repair them, then the caller retries once.
static bool repaired(FRESULT r) { return r == FR_INT_ERR && exfat_repair_dir(nullptr, CONFIG_DIR, 1) > 0; }
static FRESULT open_config(FIL* f, const char* path, BYTE mode) {
    FRESULT r = f_open(f, path, mode);
    if (repaired(r)) r = f_open(f, path, mode);
    return r;
}
void ConfigStore::forget_slots_for_tests() { s_generation = 0; s_newest = 1; }

bool ConfigStore::release() {
    if (!opened_) return true;
    if (f_close(&file_) != FR_OK) { error_ = "CONFIG CLOSE FAILED"; return false; }
    opened_ = false;
    return true;
}

// Reads a whole slot into the scratch area and validates it. size 0: no slot.
bool ConfigStore::read_slot(int slot, unsigned& size, uint32_t& generation) {
    size = 0; generation = 0;
    if (!release()) return false;
    FRESULT r = open_config(&file_, slot_names[slot], FA_READ);
    if (r == FR_NO_FILE || r == FR_NO_PATH) return true;
    if (r != FR_OK) { error_ = "CONFIG READ FAILED"; return false; }
    opened_ = true;
    FSIZE_t total = f_size(&file_);
    bool ok = total >= HEADER + 4 && total <= MAX_BYTES && total <= capacity_ &&
        (total - HEADER - 4) % RECORD == 0;
    unsigned done = 0;
    while (ok && done < total) {
        UINT got = 0;
        if (f_read(&file_, data_ + done, unsigned(total) - done, &got) != FR_OK || !got) ok = false;
        done += got;
    }
    if (!release()) return false;
    unsigned n = unsigned(total);
    if (!ok || std::memcmp(data_, "GBVCFG01", 8) || get32(data_ + 12) != (n - HEADER - 4) / RECORD ||
        fnv(data_, n - 4) != get32(data_ + n - 4) || !terminated(data_ + 32, 64)) return true; // corrupt: ignored
    size = n; generation = get32(data_ + 8);
    return true;
}

// Generation from a slot's first 16 bytes; false when absent or not a config.
bool ConfigStore::peek(int slot, uint32_t& generation) {
    generation = 0;
    if (!release()) return false;
    if (open_config(&file_, slot_names[slot], FA_READ) != FR_OK) return false;
    opened_ = true;
    unsigned char head[16];
    UINT got = 0;
    bool ok = f_read(&file_, head, 16, &got) == FR_OK && got == 16 && !std::memcmp(head, "GBVCFG01", 8);
    if (!release()) return false;
    if (ok) generation = get32(head + 8);
    return ok;
}

// Newest valid slot into scratch (without its checksum); size 0 = none.
bool ConfigStore::read_newest(unsigned& size) {
    size = 0;
    if (capacity_ < MAX_BYTES) { error_ = "CONFIG SCRATCH"; return false; }
    uint32_t ga = 0, gb = 0;
    bool pa = peek(0, ga), pb = peek(1, gb);
    if (opened_) return false;
    int newest = pb && (!pa || gb > ga) ? 1 : 0;
    uint32_t generation = 0;
    if (!read_slot(newest, size, generation)) return false;
    if (!size && (newest ? pa : pb)) {
        newest ^= 1;   // the newer slot is damaged: fall back to the other
        if (!read_slot(newest, size, generation)) return false;
    }
    if (!size) { s_generation = (ga > gb ? ga : gb); s_newest = pa || pb ? newest : 1; return true; }
    s_newest = newest; s_generation = ga > gb ? ga : gb;
    size -= 4;
    // Drop records this version cannot use (keeps the rest of the file).
    unsigned count = get32(data_ + 12), kept = 0;
    for (unsigned i = 0; i < count; ++i) {
        unsigned char* r = data_ + HEADER + i * RECORD;
        if (!record_ok(r)) continue;
        if (kept != i) std::memmove(data_ + HEADER + kept * RECORD, r, RECORD);
        ++kept;
    }
    put32(data_ + 12, kept);
    size = HEADER + kept * RECORD;
    return true;
}

bool ConfigStore::write(unsigned size) {
    if (!release()) return false;
    int slot = s_newest ^ 1;
    put32(data_ + 8, s_generation + 1);
    put32(data_ + size, fnv(data_, size));
    FRESULT d = f_mkdir(CONFIG_DIR);
    if (repaired(d)) d = f_mkdir(CONFIG_DIR);
    if (d != FR_OK && d != FR_EXIST) { error_ = "CONFIG FOLDER FAILED"; return false; }
    if (open_config(&file_, slot_names[slot], FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) { error_ = "CONFIG SAVE FAILED"; return false; }
    opened_ = true;
    UINT got = 0;
    bool ok = f_write(&file_, data_, size + 4, &got) == FR_OK && got == size + 4 && f_sync(&file_) == FR_OK;
    if (!release()) ok = false;
    if (!ok) { error_ = "CONFIG SAVE FAILED"; return false; }
    s_newest = slot; ++s_generation;
    error_ = "";
    return true;
}

int ConfigStore::find(unsigned size, const char* list) const {
    unsigned count = size ? get32(data_ + 12) : 0;
    for (unsigned i = 0; i < count; ++i)
        if (same_name(reinterpret_cast<const char*>(data_ + HEADER + i * RECORD), list)) return int(i);
    return -1;
}

bool ConfigStore::load_list(const char* list, ListRecord& out) {
    out = ListRecord();
    unsigned size = 0;
    if (!name_ok(list) || !read_newest(size)) return false;
    int i = find(size, list);
    if (i < 0) return false;
    const unsigned char* r = data_ + HEADER + unsigned(i) * RECORD;
    const char* front = reinterpret_cast<const char*>(r + 64);
    if (front[0]) out.pair.set(front, reinterpret_cast<const char*>(r + 76));
    out.settings.prefer(reinterpret_cast<const char*>(r + 88));
    out.settings.mode = r[152];
    out.settings.type_first = r[153] == 1 ? 1 : 0;   // V4.4.3; older files hold 0
    return true;
}

bool ConfigStore::save_list(const char* list, const ListRecord& record) {
    if (!name_ok(list) || !record.settings.valid()) { error_ = "CONFIG: INVALID LIST"; return false; }
    unsigned size = 0;
    if (!read_newest(size)) return false;
    if (!size) {
        std::memset(data_, 0, HEADER);
        std::memcpy(data_, "GBVCFG01", 8);
        size = HEADER;
    }
    uint32_t clock = get32(data_ + 16) + 1;
    put32(data_ + 16, clock);
    unsigned count = get32(data_ + 12);
    int i = find(size, list);
    unsigned char fresh[RECORD] = {};
    std::strcpy(reinterpret_cast<char*>(fresh), list);
    std::strcpy(reinterpret_cast<char*>(fresh + 64), record.pair.front);
    std::strcpy(reinterpret_cast<char*>(fresh + 76), record.pair.back);
    std::strcpy(reinterpret_cast<char*>(fresh + 88), record.settings.dictionary);
    fresh[152] = record.settings.mode;
    fresh[153] = record.settings.type_first;
    if (i >= 0) {
        unsigned char* r = data_ + HEADER + unsigned(i) * RECORD;
        if (!std::memcmp(r + 64, fresh + 64, 90)) { error_ = ""; return true; } // unchanged: no write
        std::memcpy(r + 64, fresh + 64, 90);
        put32(r + 156, clock);
        return write(size);
    }
    unsigned slot = count;
    if (count >= MAX_RECORDS) {
        // Full: the list used longest ago makes room.
        slot = 0;
        for (unsigned k = 1; k < count; ++k)
            if (get32(data_ + HEADER + k * RECORD + 156) < get32(data_ + HEADER + slot * RECORD + 156)) slot = k;
    } else {
        put32(data_ + 12, ++count);
        size += RECORD;
    }
    put32(fresh + 156, clock);
    std::memcpy(data_ + HEADER + slot * RECORD, fresh, RECORD);
    return write(size);
}

bool ConfigStore::last_list(char out[64]) {
    out[0] = 0;
    unsigned size = 0;
    if (!read_newest(size) || !size) return false;
    std::memcpy(out, data_ + 32, 64);
    out[63] = 0;
    return out[0] != 0;
}

namespace {
constexpr uint8_t OPTION_MAX[ConfigStore::OPTIONS] = {2, 1, 1, 3, 1, 1, 1};
}

int ConfigStore::option(int index) {
    if (index < 0 || index >= OPTIONS) return 0;
    unsigned size = 0;
    if (!read_newest(size) || !size) return 0;
    const uint8_t v = data_[20 + index];
    return v <= OPTION_MAX[index] ? v : 0;
}

bool ConfigStore::set_option(int index, int value) {
    if (index < 0 || index >= OPTIONS || value < 0 || value > OPTION_MAX[index]) { error_ = "CONFIG: INVALID OPTION"; return false; }
    unsigned size = 0;
    if (!read_newest(size)) return false;
    if (!size) {
        std::memset(data_, 0, HEADER);
        std::memcpy(data_, "GBVCFG01", 8);
        size = HEADER;
    } else if (data_[20 + index] == value) { error_ = ""; return true; }
    data_[20 + index] = uint8_t(value);
    return write(size);
}

bool ConfigStore::set_last_list(const char* list) {
    if (!name_ok(list)) { error_ = "CONFIG: INVALID LIST"; return false; }
    unsigned size = 0;
    if (!read_newest(size)) return false;
    if (!size) {
        std::memset(data_, 0, HEADER);
        std::memcpy(data_, "GBVCFG01", 8);
        size = HEADER;
    } else if (same_name(reinterpret_cast<const char*>(data_ + 32), list) &&
               !std::strcmp(reinterpret_cast<const char*>(data_ + 32), list)) {
        error_ = "";
        return true;
    }
    std::memset(data_ + 32, 0, 64);
    std::strcpy(reinterpret_cast<char*>(data_ + 32), list);
    return write(size);
}
