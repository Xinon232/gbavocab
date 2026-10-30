// render.h — UI rendering layer
// Step 5i: single 8x16 UI font (gbamp3 5x7 glyphs, include/ui_sprite_font.h) for all UI text.

#pragma once

#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"

#include "text_layout.h"
#include "flashcard_font.h"

#include "vocab.h"
#include "flashcard_angle.h"
#include "save_status.h"
#include "state.h"  // for State::Side enum

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Update the display with the current word and field counts.
    // Called every frame.
    void update(const VocabFile& vf, int current_line_idx, int current_field,
                const LineBuf& current,
                State::Side active_side,    // which side of the pair to show
                bool alternate_mode,        // direction mode 3 indicator
                bool show_answer,          // R held → also show the other side
                bool field_is_empty,       // current box has no words
                bool feedback_active);     // held A/B keeps flash active


    void update_message(const char* text);
    void set_notice(const char* text) { notice = text; }

    // Show save progress or a persistent failure indicator.
    void set_save_status(SaveStatus status);
    void set_save_error(const char* error) { save_error = error; answer_begin = answer_end = 0; }

    // Trigger a flash. flash_green() = A press. flash_red() = B press.
    void flash_green();
    void flash_red();

    void reset();

    // The Shuffle question on a turned learning screen (Settings > Flashcard
    // angle not Normal): entries_draw_choice's rows, made of sprites so they
    // turn with the cards. selected: the highlighted item.
    void draw_choice(const char* title, const char* const* lines, int line_count,
                     const char* const* items, int item_count, int selected);

private:
    // Unchanged sprite UI; compact body providers preserve field-bank policy.
    bn::sprite_text_generator small_gen;
    FlashcardFont latin_gen;
    FlashcardFont greek_cyrillic_gen;
    FlashcardFont japanese_gen;
    FlashcardFont cjk_gen;
    FlashcardFont hangul_gen;

    bn::vector<bn::sprite_ptr, 256> text_sprites;
    // Range of answer sprites in the current full frame; no extra tiles cached.
    uint32_t rendered_rejected_rows = 0;
    int rendered_line_count = 0;
    int answer_begin = 0;
    int answer_end = 0;

    CardLayout body_layout;
    char body_text[2][VOCAB_LINE_MAX] = {};

    FlashcardFont& font_for(const char* text);
    const char* notice = nullptr;
    const char* save_error = "SD I/O ERROR";
    uint32_t notified_rejected_rows = 0;
    uint16_t learning_notice_frames = 0;
    bool learning_notice_error = false;
    int last_line_idx;
    int last_field;
    State::Side last_active_side;
    bool last_alternate_mode;
    char mode_text[48] = {};
    bool last_show_answer;
    bool last_field_is_empty;
    int last_counts[5];
    SaveStatus save_status;
    SaveStatus last_save_status;
    int flash_timer_frames;
    int flash_color;  // 0 = none, 1 = green, 2 = red
    // Settings > Flashcard angle the screen was laid out for.
    FlashcardAngle angle = FlashcardAngle::normal;
    // Settings > Hide elements the screen was drawn with (bit per
    // HiddenElement: mode, word count, boxes). Hidden parts leave their
    // place empty; nothing else moves.
    uint8_t hidden = 0;
    void turn(int first);

    void render_full(const VocabFile& vf, int current_line_idx, int current_field,
                     const LineBuf& current,
                     State::Side active_side, bool alternate_mode, bool show_answer,
                     bool field_is_empty);
};
