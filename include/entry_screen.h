#pragma once
#include "render.h"
class EntryEditor;
// Main-menu dictionary add only: the list editor is inactive on this route.
EntryEditor& entry_draft_editor();
void run_entry_screen(Renderer& renderer, State& state, VocabFile& vf,
                      const char* fallback, int used, const char* front=nullptr,const char* back=nullptr);
// Saves the active list exactly as learning's Start does (main.cpp), keeping
// the learning position; false leaves the list unsaved.
bool app_save_active_list(State& state);
