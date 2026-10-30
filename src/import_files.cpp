#include "import_files.h"
#include "ff.h"
#include <cstring>

namespace {
constexpr const char* LIBRARY = "/gbavocab";
bool same_ignoring_case(const char* a, const char* b) {
    for (; *a && *b; ++a, ++b) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x = char(x + 32);
        if (y >= 'A' && y <= 'Z') y = char(y + 32);
        if (x != y) return false;
    }
    return !*a && !*b;
}
bool ends_with(const char* name, const char* ext) {
    unsigned n = unsigned(std::strlen(name)), e = unsigned(std::strlen(ext));
    return n > e && same_ignoring_case(name + n - e, ext);
}
bool exists(const char* path) { FILINFO info; return f_stat(path, &info) == FR_OK; }
}

bool import_join(char (&out)[IMPORT_PATH_MAX], const char* folder, const char* name) {
    unsigned a = unsigned(std::strlen(folder)), b = unsigned(std::strlen(name));
    bool slash = !a || folder[a - 1] != '/';
    if (!b || a + slash + b >= IMPORT_PATH_MAX) return false;
    std::memmove(out, folder, a);   // out may be folder
    if (slash) out[a++] = '/';
    std::memcpy(out + a, name, b + 1);
    return true;
}

bool import_parent(char (&path)[IMPORT_PATH_MAX]) {
    int n = int(std::strlen(path));
    while (n > 1 && path[n - 1] == '/') --n;
    if (n <= 1) { std::strcpy(path, "/"); return false; }
    while (n > 0 && path[n - 1] != '/') --n;
    if (n > 1) --n;
    path[n > 0 ? n : 1] = 0;
    if (!n) path[0] = '/';
    return true;
}

const char* import_leaf(const char* path) {
    const char* leaf = path;
    for (const char* p = path; *p; ++p) if (*p == '/' && p[1]) leaf = p + 1;
    return *leaf == '/' ? leaf + 1 : leaf;
}

bool import_is_txt(const char* name) { return ends_with(name, ".txt"); }
bool import_is_dict(const char* name) { return ends_with(name, ".dict"); }
bool import_in_library(const char* folder) { return same_ignoring_case(folder, LIBRARY); }

bool import_listed(const char* folder, const char* name, bool is_folder, bool hidden) {
    if (hidden || !name[0] || name[0] == '.') return false;
    if (!is_folder) return import_is_txt(name) || import_is_dict(name);
    return !(import_in_library(folder) && same_ignoring_case(name, "GBAVOCAB.SYS"));
}

ImportName import_target_name(const char* name, unsigned limit, char (&out)[IMPORT_NAME_MAX]) {
    unsigned n = unsigned(std::strlen(name));
    if (!n || n > limit || n >= IMPORT_NAME_MAX) return ImportName::too_long;
    char path[IMPORT_PATH_MAX];
    if (!import_join(path, LIBRARY, name)) return ImportName::too_long;
    if (!exists(path)) { std::strcpy(out, name); return ImportName::free; }
    const char* dot = std::strrchr(name, '.');
    unsigned base = dot ? unsigned(dot - name) : n;
    for (int k = 2; k <= 99; ++k) {
        char suffix[8] = " (";
        char* p = suffix + 2;
        if (k >= 10) *p++ = char('0' + k / 10);
        *p++ = char('0' + k % 10); *p++ = ')'; *p = 0;
        unsigned total = base + unsigned(std::strlen(suffix)) + (n - base);
        if (total > limit || total >= IMPORT_NAME_MAX) return ImportName::too_long;
        std::memcpy(out, name, base);
        std::strcpy(out + base, suffix);
        std::strcat(out, name + base);
        if (!import_join(path, LIBRARY, out)) return ImportName::too_long;
        if (!exists(path)) return ImportName::numbered;
    }
    return ImportName::none;
}

// One pass over the folder: counts folders and files and keeps names
// first .. first + WINDOW - 1 (folders come first). folders < 0: count only.
bool ImportFolder::read(int first) {
    DIR dir;
    FILINFO info;
    if (f_opendir(&dir, folder_) != FR_OK) return false;
    int folders = 0, files = 0, kept = 0;
    const int known = first < 0 ? -1 : folders_;
    bool ok = true;
    for (;;) {
        if (f_readdir(&dir, &info) != FR_OK) { ok = false; break; }
        if (!info.fname[0]) break;
        const bool is_folder = info.fattrib & AM_DIR;
        if (!import_listed(folder_, info.fname, is_folder, info.fattrib & (AM_HID | AM_SYS))) continue;
        const int index = is_folder ? folders++ : known + files++;
        if (known >= 0 && index >= first && index - first < WINDOW) {
            char* slot = names_ + (index - first) * IMPORT_NAME_MAX;
            std::strncpy(slot, info.fname, IMPORT_NAME_MAX - 1); slot[IMPORT_NAME_MAX - 1] = 0;
            if (index - first + 1 > kept) kept = index - first + 1;
        }
    }
    if (f_closedir(&dir) != FR_OK) ok = false;
    if (!ok) return false;
    if (known < 0) { folders_ = folders; count_ = folders + files; return true; }
    if (folders != folders_ || folders + files != count_) return false;   // changed meanwhile
    first_ = first; kept_ = kept;
    return true;
}

bool ImportFolder::open(const char* folder) {
    count_ = folders_ = first_ = kept_ = 0;
    if (std::strlen(folder) >= IMPORT_PATH_MAX) return false;
    std::strcpy(folder_, folder);
    return read(-1) && read(0);
}

const char* ImportFolder::name(int i) {
    if (i < 0 || i >= count_) return nullptr;
    if (i < first_ || i >= first_ + kept_) {
        int first = i - WINDOW / 2;
        if (first < 0) first = 0;
        if (!read(first) || i - first_ >= kept_) { kept_ = 0; return nullptr; }
    }
    return names_ + (i - first_) * IMPORT_NAME_MAX;
}

ImportCopy import_copy(const char* source, const char* target, unsigned char* buffer, unsigned size,
                       void (*progress)(void*, int), void* context) {
    FRESULT made = f_mkdir(LIBRARY);
    if (made != FR_OK && made != FR_EXIST) return ImportCopy::write_failed;
    FIL in, out;
    if (f_open(&in, source, FA_READ) != FR_OK) return ImportCopy::read_failed;
    if (f_open(&out, target, FA_WRITE | FA_CREATE_NEW) != FR_OK) { f_close(&in); return ImportCopy::write_failed; }
    const FSIZE_t total = f_size(&in);
    FSIZE_t done = 0;
    ImportCopy result = ImportCopy::copied;
    int shown = -1;
    while (done < total) {
        const int percent = int(uint64_t(done) * 100 / total);
        if (progress && percent != shown) { progress(context, percent); shown = percent; }
        UINT want = total - done > size ? size : UINT(total - done), got = 0, put = 0;
        if (f_read(&in, buffer, want, &got) != FR_OK || got != want) { result = ImportCopy::read_failed; break; }
        if (f_write(&out, buffer, got, &put) != FR_OK || put != got) { result = ImportCopy::write_failed; break; }
        done += got;
    }
    f_close(&in);
    if (f_sync(&out) != FR_OK && result == ImportCopy::copied) result = ImportCopy::write_failed;
    if (f_close(&out) != FR_OK && result == ImportCopy::copied) result = ImportCopy::write_failed;
    if (result != ImportCopy::copied) f_unlink(target);
    else if (progress) progress(context, 100);
    return result;
}

uint32_t import_count_rows(const char* source, unsigned char* buffer, unsigned size, uint32_t stop, bool& ok) {
    ok = false;
    FIL f;
    if (f_open(&f, source, FA_READ) != FR_OK) return 0;
    uint32_t rows = 0;
    bool content = false, comment = false, start = true;
    for (;;) {
        UINT got = 0;
        if (f_read(&f, buffer, size, &got) != FR_OK) { f_close(&f); return rows; }
        if (!got) break;
        for (UINT i = 0; i < got; ++i) {
            const unsigned char c = buffer[i];
            if (c == '\n') { if (content && !comment) ++rows; content = comment = false; start = true; continue; }
            if (c == ' ' || c == '\t' || c == '\r' || c == 0xef || c == 0xbb || c == 0xbf) continue;
            if (start && c == '#') comment = true;
            start = false; content = true;
        }
        if (rows > stop) break;
    }
    if (content && !comment) ++rows;
    f_close(&f);
    ok = true;
    return rows;
}
