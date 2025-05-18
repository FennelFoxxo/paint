#pragma once

#include "gui_resource.hpp"

#include <imgui.h>

#include <SDL3/SDL.h>

struct State {
    GuiResource* gui_resource;
    
    bool should_quit = false;
    bool window_minimized;
    int window_width, window_height;
    bool mouse_down;
    int mouse_x, mouse_y;
    float framerate;
    
    
    bool show_another_window = false;
    ImVec4 clear_color;
    
    SDL_Texture* canvas;
};