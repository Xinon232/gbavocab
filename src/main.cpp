// GBA Vocab Trainer — main entry
// Step 5c: light blue background, single-word prompt, R reveals
// answer, D-pad L/R switches boxes, L cycles 3 direction modes.

#include "bn_core.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include <cstring>

#include "vocab.h"
#include "render.h"
#include "state.h"
#include "vocab_file_io.h"
#include "entry_shortcuts.h"
#include "entry_screen.h"
#include "home_screen.h"
#include "dictionary_screen.h"
#include "import_screen.h"
#include "entries_screen.h"

#include "ui_sprite_font.h"

BN_DATA_EWRAM_BSS VocabFile g_vocab_file;

BN_DATA_EWRAM_BSS char g_builtin_vocab[VOCAB_FILE_BUFFER_LEN];
BN_DATA_EWRAM_BSS char g_export_buffer[VOCAB_EXPORT_BUFFER_LEN];
BN_DATA_EWRAM_BSS int g_builtin_vocab_used = 0;
BN_DATA_EWRAM_BSS int g_export_buffer_used = 0;

// On the GBA a list is always streamed from the SD card: the legacy fallback
// and export buffers stay idle, so the list screens borrow them.
char* entries_row_cache() { return g_builtin_vocab; }
unsigned char* entries_order_scratch() { return reinterpret_cast<unsigned char*>(g_export_buffer); }
static_assert(VOCAB_FILE_BUFFER_LEN >= 2048 && VOCAB_EXPORT_BUFFER_LEN >= 4096, "list screen scratch");

static bool load_selected_vocab(const char* filename)
{
    bool loaded = vocab_file_load(filename, g_vocab_file, g_builtin_vocab,
                                  VOCAB_FILE_BUFFER_LEN, g_builtin_vocab_used);
    if (loaded) vocab_file_remember_last(filename);   // LOAD LIST cursor at the next start
    return loaded;
}

static int grouped_save_index_for_line(const VocabFile& vf, int old_idx)
{
    if (old_idx < 0 || old_idx >= vf.line_count) {
        return -1;
    }

    uint8_t field = vf.field[old_idx];
    if (field < 1 || field > 5) {
        return -1;
    }

    int new_idx = 0;
    for (int i = 0; i < vf.line_count; ++i) {
        uint8_t candidate_field = vf.field[i];
        if (candidate_field < field || (candidate_field == field && i < old_idx)) {
            ++new_idx;
        }
    }
    return new_idx;
}

static State::InputState read_input()
{
    State::InputState in;
    in.a_pressed       = bn::keypad::a_pressed();
    in.b_pressed       = bn::keypad::b_pressed();
    in.a_held          = bn::keypad::a_held();
    in.b_held          = bn::keypad::b_held();
    in.r_held          = bn::keypad::r_held();
    in.l_pressed       = bn::keypad::l_pressed();
    in.l_held          = bn::keypad::l_held();
    in.start_pressed   = bn::keypad::start_pressed();
    in.select_pressed  = bn::keypad::select_pressed();
    // Settings > Flashcard angle: the D-pad turns with the cards.
    const TurnedPad pad = turn_pad({bn::keypad::up_pressed(), bn::keypad::down_pressed(),
                                    bn::keypad::left_pressed(), bn::keypad::right_pressed()},
                                   vocab_file_flashcard_angle());
    in.left_pressed    = pad.left;
    in.right_pressed   = pad.right;
    in.up_pressed      = pad.up;
    in.down_pressed    = pad.down;
    return in;
}

static void render_current_frame(Renderer& renderer, State& state)
{
    // The old file browser, switch and shuffle screens are gone: Select opens
    // the main menu and the shuffle question is a gbamp3 choice (main loop).


    int idx = state.current_line_idx();
    if (idx < 0 || idx >= g_vocab_file.line_count) idx = 0;

    LineBuf current = {};
    bool field_empty = !state.feedback_active() && state.current_field_is_empty(g_vocab_file);
    if (vocab_file_show(g_vocab_file, g_builtin_vocab, g_builtin_vocab_used,
                        idx, current) || field_empty) {
        renderer.update(g_vocab_file, idx, state.current_field(),
                        current, state.active_side(), state.direction_mode() == 3,
                        state.show_answer(), field_empty, state.feedback_active());
    } else {
        renderer.update_message("READ ERROR / text too long");
    }
}

static bool home_save(void* context)
{
    auto& state = *static_cast<State*>(context);
    int grouped_idx = grouped_save_index_for_line(g_vocab_file, state.current_line_idx());
    bool saved = vocab_file_save_grouped(g_vocab_file, g_builtin_vocab, g_builtin_vocab_used,
        g_export_buffer, VOCAB_EXPORT_BUFFER_LEN, g_export_buffer_used);
    if (vocab_file_save_installed_index()) state.restore_current_line_index(g_vocab_file, grouped_idx);
    return saved;
}

bool app_save_active_list(State& state) { return home_save(&state); }

// V4.4.3: set by the New List action; the list's languages are asked next.
static bool s_list_created = false;

static void open_home(Renderer& renderer, State& state)
{
    HomeActions actions{&state, home_save,
        [](void*, const char* filename) { return load_selected_vocab(filename); },
        [](void*, const char* filename) {
            bool created = vocab_file_create(filename, g_vocab_file);
            if (created) vocab_file_remember_last(filename);
            s_list_created = created;
            return created;
        }};
    // The dictionary opened last from here: B in the main menu returns to it
    // ("B: Resume active dictionary") until a list is resumed or opened.
    char active_dictionary[64] = {};
    while(true) {
        bool dictionary_requested=false, import_requested=false, dictionary_resumed=false;
        if (run_home_screen(renderer, g_vocab_file, actions, &dictionary_requested, false, &import_requested,
                            active_dictionary, &dictionary_resumed)) {
            state = State(g_vocab_file.settings.mode); // Only a successful load/create replaces navigation.
            renderer.set_notice(nullptr);
            renderer.set_save_status(SaveStatus::IDLE);
            if (s_list_created) {
                // New List: its languages (a dictionary, typed codes or Later).
                s_list_created = false;
                dictionary_new_list(renderer, g_vocab_file);
                if (g_vocab_file.pair_dirty) vocab_file_save_settings(g_vocab_file);
            }
        }
        if(import_requested){run_import_screen(renderer);continue;}   // back to home afterwards
        if(!dictionary_requested)break;
        // Main menu > Dictionaries: choose one (skipped with a single file),
        // search it; B in the search returns to the choice.
        char chosen[64]={};bool single=false;
        if(dictionary_resumed){std::strcpy(chosen,active_dictionary);single=true;}   // B in its search: the main menu
        while(dictionary_resumed||dictionary_pick(renderer,chosen,single)) {
            dictionary_resumed=false;
            std::strcpy(active_dictionary,chosen);
            DictionaryResult result;
            if(run_dictionary_screen(renderer,nullptr,result,chosen)) {
                // Existing guarded browser owns Save/Discard/Cancel before switching.
                if(run_home_screen(renderer, g_vocab_file, actions, nullptr, true)) {
                    state=State(g_vocab_file.settings.mode);
                    if(dictionary_accept_pair(renderer,g_vocab_file,result)) {
                        if(g_vocab_file.pair_dirty) vocab_file_save_settings(g_vocab_file);
                        run_entry_screen(renderer,state,g_vocab_file,g_builtin_vocab,g_builtin_vocab_used,result.front,result.back);
                    }
                }
                break;
            }
            if(single)break;
        }
    }
}

int main()
{
    bn::core::init();
    // White at once: starting the SD card below takes a while on a real cart,
    // and the engine's default backdrop is black.
    bn::bg_palettes::set_transparent_color(bn::color(31, 31, 31));
    bn::core::update();

    vocab_file_init();
    entries_set_letter_box(LetterBox(vocab_file_letter_box()));   // Secret Settings

    Renderer renderer;
    State state;
    open_home(renderer, state);
    int last_scene = state.scene();
    EntryShortcuts shortcuts;

    while(true)
    {
        State::InputState in = read_input();
        bool save_requested = false;
        if ((state.scene() == 0 || state.feedback_active()) && g_vocab_file.loaded) {
            auto action = shortcuts.update(bn::keypad::start_held(), bn::keypad::select_held());
            in.start_pressed = false;
            in.select_pressed = false;
            if (action == EntryShortcuts::Action::menu) {
                // Secret Settings > Save list on menu (default On): the open list
                // is saved first (the menu reuses the save's memory, so this
                // save cannot run behind it). A failure still opens the menu;
                // switching lists then asks Save / Discard / Cancel as before.
                if (vocab_file_save_on_menu() && vocab_file_loaded_from_sd() &&
                    (vocab_any_dirty(g_vocab_file) || g_vocab_file.array_generation || g_vocab_file.pair_dirty)) {
                    int idx = state.current_line_idx();
                    int grouped_idx_menu = idx >= 0 && idx < g_vocab_file.line_count &&
                        (vocab_any_dirty(g_vocab_file) || g_vocab_file.array_generation) ? grouped_save_index_for_line(g_vocab_file, idx) : -1;
                    renderer.set_save_status(SaveStatus::SAVING);
                    render_current_frame(renderer, state);
                    bn::core::update();
                    bool saved = vocab_file_save_grouped(g_vocab_file, g_builtin_vocab, g_builtin_vocab_used,
                                                         g_export_buffer, VOCAB_EXPORT_BUFFER_LEN, g_export_buffer_used);
                    if (vocab_file_save_installed_index()) state.restore_current_line_index(g_vocab_file, grouped_idx_menu);
                    renderer.set_save_error(vocab_file_last_error());
                    renderer.set_save_status(saved ? SaveStatus::IDLE : SaveStatus::FAILED);
                }
                open_home(renderer, state);
                shortcuts.suppress_until_release();
                last_scene = state.scene();
                continue;
            }
            save_requested = action == EntryShortcuts::Action::save && state.scene() == 0;
            if (action == EntryShortcuts::Action::editor) {
                run_entry_screen(renderer, state, g_vocab_file, g_builtin_vocab, g_builtin_vocab_used);
                shortcuts.suppress_until_release();
                continue;
            }
        } else {
            shortcuts.suppress_until_release();
            if (!g_vocab_file.loaded && state.scene() == 0 && in.select_pressed) {
                open_home(renderer, state);
                last_scene = state.scene();
                continue;
            }
        }
        state.update(g_vocab_file, in);

        // Down in learning: the shuffle question as a gbamp3 choice (No first),
        // answered into the state machine as its A / B.
        if (state.shuffle_confirm_active()) {
            renderer.reset();
            bn::core::update(); bn::core::update();
            int answer = -1;
            char title[24] = "Shuffle Box ";
            title[12] = char('0' + state.current_field()); title[13] = '?'; title[14] = 0;
            const char* lines[] = {"The cards of this box get", "a new random order.", ""};
            static const char* const items[] = {"No", "Yes"};
            if (vocab_file_flashcard_angle() == FlashcardAngle::normal) {
                Canvas c;
                answer = run_choice(c, title, lines, 3, items, 2);
            } else {
                // Turned with the cards; Up / Down as seen on the turned screen.
                int selected = 0;
                for (bool redraw = true;; bn::core::update()) {
                    if (redraw) renderer.draw_choice(title, lines, 3, items, 2, selected);
                    redraw = false;
                    if (bn::keypad::b_pressed()) break;
                    if (bn::keypad::a_pressed()) { answer = selected; break; }
                    const TurnedPad pad = turn_pad({bn::keypad::up_pressed(), bn::keypad::down_pressed(),
                                                    bn::keypad::left_pressed(), bn::keypad::right_pressed()},
                                                   vocab_file_flashcard_angle());
                    if (pad.up || pad.down) { selected = 1 - selected; redraw = true; }
                }
                renderer.reset();
            }
            while (bn::keypad::a_held() || bn::keypad::b_held()) bn::core::update();
            bn::core::update(); bn::core::update();
            State::InputState reply;
            reply.a_pressed = answer == 1;
            reply.b_pressed = answer != 1;
            state.update(g_vocab_file, reply);
            renderer.reset();
            last_scene = state.scene();
            continue;
        }

        if (save_requested && state.scene() == 0) {
            int grouped_idx_after_save = -1;
            int idx_before_save = state.current_line_idx();
            if (vocab_file_loaded_from_sd() &&
                (vocab_any_dirty(g_vocab_file) || g_vocab_file.array_generation) &&
                idx_before_save >= 0 && idx_before_save < g_vocab_file.line_count) {
                grouped_idx_after_save = grouped_save_index_for_line(g_vocab_file, idx_before_save);
            }

            renderer.set_save_status(SaveStatus::SAVING);
            render_current_frame(renderer, state);
            bn::core::update();

            bool saved = vocab_file_save_grouped(g_vocab_file, g_builtin_vocab, g_builtin_vocab_used,
                                                 g_export_buffer, VOCAB_EXPORT_BUFFER_LEN,
                                                 g_export_buffer_used);
            if (vocab_file_save_installed_index()) {
                state.restore_current_line_index(g_vocab_file, grouped_idx_after_save);
            }

            renderer.set_save_error(vocab_file_last_error());
            renderer.set_save_status(saved ? SaveStatus::IDLE : SaveStatus::FAILED);
            renderer.reset();
        }

        if (state.scene() != last_scene) {
            renderer.set_notice(nullptr);
            renderer.reset();
            last_scene = state.scene();
        }

        if (state.load_request_pending()) {
            renderer.set_notice(nullptr); // fresh operation, before any new failure
            bool proceed = true;
            if (state.save_before_load()) {
                int grouped_idx = grouped_save_index_for_line(g_vocab_file, state.current_line_idx());
                renderer.update_message("Saving...");
                bn::core::update();
                proceed = vocab_file_save_grouped(g_vocab_file, g_builtin_vocab, g_builtin_vocab_used,
                    g_export_buffer, VOCAB_EXPORT_BUFFER_LEN, g_export_buffer_used);
                if (vocab_file_save_installed_index()) state.restore_current_line_index(g_vocab_file, grouped_idx);
                renderer.set_save_error(vocab_file_last_error());
                renderer.set_save_status(proceed ? SaveStatus::IDLE : SaveStatus::FAILED);
            }
            const char* filename = state.consume_load_request();
            if (proceed && load_selected_vocab(filename)) {
                state = State(g_vocab_file.settings.mode); // new list: reset navigation, undo and feedback
                renderer.set_notice(nullptr);
                renderer.set_save_status(SaveStatus::IDLE);
                renderer.reset();
            } else {
                renderer.set_notice(proceed ? "LOAD FAILED" : "SAVE FAILED - not switched");
            }
        }

        switch (state.consume_flash()) {
            case State::FLASH_GREEN: renderer.flash_green(); break;
            case State::FLASH_RED:   renderer.flash_red(); break;
            case State::FLASH_NONE:
            default: break;
        }

        render_current_frame(renderer, state);

        bn::core::update();
    }
}
