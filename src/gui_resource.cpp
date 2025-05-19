#include "gui_resource.hpp"

#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

GuiResource::GuiResource(std::string window_name, int window_width, int window_height) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO))
        // Throw error if initialization failed
        throw std::runtime_error(std::string("Error: SDL_Init(): ") + SDL_GetError());

    // Create window that is resizable
    window = SDL_CreateWindow(  window_name.c_str(), window_width, window_height, SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
        // Throw error if window creation failed
        throw std::runtime_error(std::string("Error: SDL_CreateWindow(): ") + SDL_GetError());

    // Create renderer used to draw objects to the window
    renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr)
        // Throw error if renderer creation failed
        throw std::runtime_error(std::string("Error: SDL_CreateRenderer(): ") + SDL_GetError());
    
    // Set vsync to match monitor refresh rate
    SDL_SetRenderVSync(renderer, 1);

    // Make sure our ImGui header matches with the compiled ImGui library
    IMGUI_CHECKVERSION();
    
    // Create ImGui context, holds internal ImGui state
    ImGui::CreateContext();
    
    // Get IO object, holds a struct of input/output object that changes every frame (e.g. mouse, keyboard, fps info)
    io = &ImGui::GetIO();

    // Setup ImGui style to use dark mode
    ImGui::StyleColorsDark();

    // Setup SDL backend for ImGui
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

GuiResource::~GuiResource() {
    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}