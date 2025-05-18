#include "gui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>

void guiUpdateStateMeta(State* state) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
            state->should_quit = true;
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(state->gui_resource->window))
            state->should_quit = true;
    }
    
    state->window_minimized = SDL_GetWindowFlags(state->gui_resource->window) & SDL_WINDOW_MINIMIZED;
    
    SDL_GetWindowSize(state->gui_resource->window, &state->window_width, &state->window_height);
    
    state->mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    ImVec2 mouse_pos = ImGui::GetMousePos();
    state->mouse_x = (int)mouse_pos.x;
    state->mouse_y = (int)mouse_pos.y;
    
    state->framerate = state->gui_resource->io->Framerate;
    
}


void guiDraw(State* state) {
    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
        
    // Texture
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(state->window_width - 200, state->window_height));
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            
            ImGui::Begin("canvas", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize  | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);
            
            ImGui::Image((ImTextureID)(intptr_t)state->canvas, ImVec2((float)256, (float)256));
            
            ImGui::End();
            
            ImGui::PopStyleVar(2);
        }
        
        // Menu bar
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowPos(ImVec2(state->window_width, 0), 0, ImVec2(1, 0));
            ImGui::SetNextWindowSize(ImVec2(200, state->window_height));
            
            // Create a window called "Hello, world!" and append into it.
            ImGui::Begin("Hello, world!", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Another Window", &state->show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorPicker3("clear color", (float*)&state->clear_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel ); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / state->framerate, state->framerate);
            ImGui::Text("Size: %f", ImGui::GetWindowWidth());
            
            ImGui::End();
        }

        // 3. Show another simple window.
        if (state->show_another_window)
        {
            ImGui::Begin("Another Window", &state->show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                state->show_another_window = false;
            ImGui::End();
        }
}



void guiPresent(State* state) {
    // Rendering
    ImGui::Render();
    //SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(state->gui_resource->renderer, state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    SDL_RenderClear(state->gui_resource->renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), state->gui_resource->renderer);
    SDL_RenderPresent(state->gui_resource->renderer);
}