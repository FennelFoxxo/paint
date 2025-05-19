#pragma once

#include "state.hpp"
#include "gui.hpp"

// Initializes the state and creates some required objects e.g. canvas and icon textures
void backendInit(State* state);

// Process events that happened e.g. if user dragged mouse to draw
void backendProcess(State* state);