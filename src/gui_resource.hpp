#pragma once

#include <string>

#include <SDL3/SDL.h>
#include <imgui.h>

// Object to keep track of vital graphical resources that should be kept alive for entire program lifetime
class GuiResource {
    public:
        GuiResource(std::string window_name, int window_width, int window_height);
        ~GuiResource();
        
        // Make sure object can't be copied, else the destructor could be called twice
        GuiResource(const GuiResource&) = delete;
        void operator=(const GuiResource&) = delete;
        
        // Keep track of window, renderer, and ImGui IO object
        SDL_Window* window;
        SDL_Renderer* renderer;
        ImGuiIO* io;
};