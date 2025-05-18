#pragma once

#include "state.hpp"
#include "gui_resource.hpp"


// Updates the state with meta information about the graphical state and window, such as window events and mouse position
void guiUpdateStateMeta(State* state);
void guiDraw(State* state);
void guiPresent(State* state);