#pragma once
#include "pair_metadata.h"
#include "list_settings.h"
#include "ff.h"
#include <cstdint>

// gbavocab V4.0 central configuration. One file remembers, for every list,
// its language pair, learning mode and linked dictionary, and which list was
// opened last. Like gbamp3's STATE0/1.DAT it has two slots: a save writes the
// older slot, so the newer one survives an interrupted write. Lists stay
// plain dict.cc TXT files; nothing is written next to them.
//
// Slot layout (little endian):
//   0  "GBVCFG01"   8  u32 generation   12 u32 record count   16 u32 use clock
//   20 letter box (0 Off, 1 On, 2 only L + Up/Down; V4.4 Secret Settings)
//   21 save list on menu (0 On, 1 Off; V4.4 Secret Settings)
//   22 save after entry changes (0 On, 1 Off; V4.4 Secret Settings)
//   23 flashcard angle (0 Normal, 1 90° right, 2 90° left, 3 upside down)
//   24 hide mode, 25 hide total word count, 26 hide box indicator
//      (0 shown, 1 hidden; Settings > Hide elements, older files hold 0)
//   32 last opened list name (64 bytes, NUL padded)
//   96 records, 160 bytes each:
//      0 list name[64]  64 front[12]  76 back[12]  88 dictionary[64]
//      152 mode (1 front, 2 back, 3 alternate)
//      153 type first (0 front column, 1 back column; V4.4.3)  156 u32 last use
//   then u32 FNV-1a of every byte before it.
namespace app_config {
constexpr unsigned HEADER = 96, RECORD = 160, MAX_RECORDS = 192;
constexpr unsigned MAX_BYTES = HEADER + RECORD * MAX_RECORDS + 4;
struct ListRecord {
    PairMetadata pair;
    ListSettings settings;
};
}

// Stateless apart from the slot generation. Uses the caller's (spare) FIL and
// a borrowed scratch area of at least app_config::MAX_BYTES bytes.
class ConfigStore {
public:
    ConfigStore(FIL& file, bool& opened, unsigned char* scratch, unsigned capacity)
        : file_(file), opened_(opened), data_(scratch), capacity_(capacity) {}
    // True when the list has a record; out is left at defaults otherwise.
    // A missing or unreadable configuration reads as "no record".
    bool load_list(const char* list, app_config::ListRecord& out);
    bool save_list(const char* list, const app_config::ListRecord& record);
    bool last_list(char out[64]);
    bool set_last_list(const char* list);
    // Secret Settings, one byte each from header offset 20; 0 is the default.
    // 0 letter box (0 Off, 1 On, 2 only L + Up/Down), 1 save on menu (0 On,
    // 1 Off), 2 save after entry changes (0 On, 1 Off), 3 flashcard angle
    // (FlashcardAngle: 0 Normal .. 3 upside down), 4 hide mode, 5 hide
    // total word count, 6 hide box indicator (0 shown, 1 hidden).
    static constexpr int OPTIONS = 7;
    int option(int index);
    bool set_option(int index, int value);
    int letter_box() { return option(0); }
    bool set_letter_box(int mode) { return set_option(0, mode); }
    const char* error() const { return error_; }
    // Directory holding the two slots (created on first save).
    static const char* directory();
    static void forget_slots_for_tests();
private:
    bool read_newest(unsigned& size);
    bool read_slot(int slot, unsigned& size, uint32_t& generation);
    bool peek(int slot, uint32_t& generation);
    bool write(unsigned size);
    bool release();
    int find(unsigned size, const char* list) const;
    FIL& file_;
    bool& opened_;
    unsigned char* data_;
    unsigned capacity_;
    const char* error_ = "";
};
