#pragma once

#include <string>

#include <SDL3/SDL.h>
#include <imgui.h>

class GuiResource {
    public:
        GuiResource(std::string window_name, int window_width, int window_height);
        ~GuiResource();
        
        GuiResource(const GuiResource&) = delete;
        void operator=(const GuiResource&) = delete;
        
        SDL_Window* window;
        SDL_Renderer* renderer;
        ImGuiIO* io;
};