#pragma once

// V4.4 home: the main menu (gbamp3 style: Dictionaries, then every list A to Z)
// is a list screen; this class keeps the decisions and the other pages
// (Help with New List and Save settings, Controls, Credits, the unsaved-list
// question).
constexpr int HOME_HELP_PAGES = 23;
constexpr int HOME_CREDIT_PAGES = 7;   // the last one leads to Secret Settings (A)
constexpr int HOME_PAGE_LINES = 8;   // one per list row; "#": grey subheading
// Settings menu ("Help" before) topics: New List, Flashcard angle,
// Save settings and Hide elements first, Credits last.
constexpr int HOME_HELP_TOPICS = 12;   // New List, Flashcard angle, Save settings, Hide elements, 7 topics, Credits

class HomeScreen {
public:
    explicit HomeScreen(bool destination=false):destination_(destination){}
    enum class Page { main, new_list, confirm, help, controls, credits, secret, save_settings, flashcard_angle, hide_elements };
    enum class Key { a, b, up, down, left, right, select, start };
    enum class Request { none, resume, load, create, dictionary, import };
    Page page() const { return page_; }
    int help_page() const { return help_page_; }
    // Help menu (Select): the chosen topic and its pages.
    int topic() const { return topic_; }
    Request request() const { return request_; }
    bool save_first() const { return save_first_; }
    bool destination() const { return destination_; }
    // Main menu choices.
    void open_dictionaries() { if(!destination_) request_ = Request::dictionary; }
    // A on a list; the open list itself only returns to it.
    void open_list(bool active, bool dirty, bool is_active);
    void back(bool active) { if(active || destination_) request_ = Request::resume; }
    void open_help() { if(!destination_) { topic_ = 0; page_ = Page::help; } }
    void open_import() { if(!destination_) request_ = Request::import; }
    // Help list: the topic under the cursor.
    void choose_topic(int topic) { if(page_ == Page::help && topic >= 0 && topic < HOME_HELP_TOPICS) topic_ = topic; }
    // Keys on the other pages.
    void press(Key key, bool active, bool dirty);
    void finish(bool success);
private:
    Page page_ = Page::main;
    Page origin_ = Page::main;
    int help_page_ = 0;
    int topic_ = 0;
    Request request_ = Request::none;
    bool save_first_ = false;
    bool destination_ = false;
};

const char* home_topic_name(int topic);
int home_topic_first(int topic);   // first page (Controls or Credits pages)
int home_topic_pages(int topic);
bool home_topic_credits(int topic);
bool home_topic_new_list(int topic);
bool home_topic_flashcard_angle(int topic);
bool home_topic_save_settings(int topic);
bool home_topic_hide_elements(int topic);
const char* home_help_heading(int page);
const char* home_help_line(int page, int line);
const char* home_credit_heading(int page);
const char* home_credit_line(int page, int line);
// Explanations under the line: Secret Settings (Letter Box), Save settings,
// Flashcard angle and Hide elements.
constexpr int HOME_SECRET_LINES = 4;
const char* home_secret_line(int line);
constexpr int HOME_SAVE_LINES = 5;
const char* home_save_line(int line);
constexpr int HOME_ANGLE_LINES = 4;
const char* home_angle_line(int line);
constexpr int HOME_HIDE_LINES = 5;
const char* home_hide_line(int line);

class Renderer;
struct VocabFile;
// Callbacks own storage and the learning State, including save index remapping.
struct HomeActions {
    void* context;
    bool (*save)(void*);
    bool (*load)(void*, const char*);
    bool (*create)(void*, const char*);
};
enum class HomeResult { opened, save_failed, open_failed };
HomeResult home_apply_request(const HomeScreen&, HomeActions, const char* filename);
// True after a successful switch; false when resuming the retained list.
// active_dictionary: the dictionary opened last from the main menu (nullptr:
// the list was last). Its footer then reads "B: Resume active dictionary" and B
// sets *dictionary_resumed (with *dictionary_requested) instead of resuming the list.
bool run_home_screen(Renderer&, VocabFile&, HomeActions, bool* dictionary_requested=nullptr, bool destination=false,
                     bool* import_requested=nullptr, const char* active_dictionary=nullptr, bool* dictionary_resumed=nullptr);
