#pragma once
// gbavocab V4.1 Import (Start on the home screen): browse the SD card from its
// root, copy TXT lists and .dict files into /gbavocab or build a .dict from a
// TXT. Folders come first, then files, each in card order (as gbareader's
// import browser). Hidden/system entries and the settings folder are left out.
#include <cstdint>

constexpr int IMPORT_PATH_MAX = 256;
constexpr int IMPORT_NAME_MAX = 64;     // FatFS long names here: 63 bytes
constexpr unsigned IMPORT_LIST_NAME_MAX = 54;  // a list must stay saveable
constexpr uint32_t IMPORT_LIST_ROWS = 15000;

bool import_join(char (&out)[IMPORT_PATH_MAX], const char* folder, const char* name);
bool import_parent(char (&path)[IMPORT_PATH_MAX]);   // false at the root
const char* import_leaf(const char* path);
bool import_is_txt(const char* name);
bool import_is_dict(const char* name);
bool import_in_library(const char* folder);          // the /gbavocab folder
bool import_listed(const char* folder, const char* name, bool is_folder, bool hidden);

// Name for a copy in /gbavocab: the same name when free, otherwise the first
// free "Name (2).ext" .. "Name (99).ext". Nothing is ever replaced.
enum class ImportName { free, numbered, too_long, none };
ImportName import_target_name(const char* name, unsigned limit, char (&out)[IMPORT_NAME_MAX]);

// The entries of one folder, names kept in a borrowed window.
class ImportFolder {
public:
    static constexpr int WINDOW = 32;
    explicit ImportFolder(char* names) : names_(names) {}   // WINDOW * IMPORT_NAME_MAX bytes
    bool open(const char* folder);
    const char* folder() const { return folder_; }
    int count() const { return count_; }
    bool is_folder(int i) const { return i >= 0 && i < folders_; }
    const char* name(int i);    // null when it cannot be reread
private:
    bool read(int first);
    char* names_;
    char folder_[IMPORT_PATH_MAX] = {};
    int count_ = 0, folders_ = 0, first_ = 0, kept_ = 0;
};

enum class ImportCopy { copied, read_failed, write_failed };
// target must not exist. A failed copy leaves no partial file.
ImportCopy import_copy(const char* source, const char* target, unsigned char* buffer, unsigned size,
                       void (*progress)(void*, int), void* context);
// Rows a list would hold: non-blank lines that are not '#' comments. Stops
// counting at stop + 1.
uint32_t import_count_rows(const char* source, unsigned char* buffer, unsigned size, uint32_t stop, bool& ok);
