#include "gui.hpp"
#include "utils.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <iostream>
// Updates the state with meta information about the graphical state and window, such as window events and mouse position
void guiUpdateStateMeta(State* state) {
    // Iterate over any events that occured since last frame e.g. keyboard/mouse input
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Let ImGui know about the event
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
            // If user clicked the X in the top right of the window
            state->should_quit = true;
    }
    
    // Save the window width and height in the state struct
    SDL_GetWindowSize(state->gui_resource->window, &state->window_width, &state->window_height);
    
    // X coordinate of the viewport (where canvas is rendered to) is always 0 since there is no menus or padding on the left
    state->viewport.x = 0;
    
    // Save some mouse information used by the backend
    state->mouse_left_down = ImGui::IsMouseDown(ImGuiMouseButton_Left); // Is left mouse button down?
    state->mouse_right_dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Right); // Is the right mouse button being used to drag?
    state->mouse_pos = ImGui::GetMousePos(); // xy mouse pos on window
    state->scroll = state->gui_resource->io->MouseWheel; // Mouse scroll wheen input - positive for scrolling up, negative for down
    state->drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
    
    // WantCaptureMouse is true if the mouse is over an ImGui window,
    //so if the mouse is over the canvas (which is rendered without ImGui windows) then it is false
    state->mouse_over_canvas = !state->gui_resource->io->WantCaptureMouse;
    
    // Framerate in FPS
    state->framerate = state->gui_resource->io->Framerate;
    
}

// Draws the bar at the top of the screen with the File and Image options
void drawMainMenuBar(State* state) {
    if (ImGui::BeginMainMenuBar()) { // Start of menu bar

        // File menu
        if (ImGui::BeginMenu("File")) {
            // "New" button
            if (ImGui::MenuItem("New")) {
                state->show_new_file_window = true; // Open window with new file options
            }
            
            // "Open" button
            if (ImGui::MenuItem("Open")) {
                //state->file_action_info.status = FileActionInfo::Open;
            }
            
            // "Save As" button
            if (ImGui::MenuItem("Save As")) {
                //state->file_action_info.status = FileActionInfo::SaveAs;
            }
            
            // "Exit" button
            if (ImGui::MenuItem("Exit")) {
                state->should_quit = true; // Quit the program
            }
            ImGui::EndMenu();
        }
        
        // Image menu
        if (ImGui::BeginMenu("Image")) {
            // "Resize" button
            if (ImGui::MenuItem("Resize")) {
                state->show_resize_window = true; // Open window with resize options
            }
            ImGui::EndMenu();
        }
        
        // Now that we've rendered the menu at the top, we can fill in some more parameters about the viewport size.
        state->viewport.y = ImGui::GetWindowHeight(); // Y coordinate is at the bottom of this window (menu bar).
        
        // Bottom of the viewport is the bottom of the window, so total height is window height - y coordinate.
        // Note that the ImVec4 struct stores its values as x, y, z, w and we're using it as x, y, width, height,
        // so the "w" represents height and not width here.
        state->viewport.w = state->window_height - ImGui::GetWindowHeight();
        
        // End of menu bar
        ImGui::EndMainMenuBar();
    }
}

// Draw the resize window if the user select Image->Resize in the menu bar
void drawResizeWindow(State* state) {
    // Exit early if window is hidden
    if (!state->show_resize_window) {
        return;
    }
    
    // Let ImGui determine best window size based on contents
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    
    // Start of window
    // Passing the window visibility flag allows you to use the "X" in the corner of the window to close it
    ImGui::Begin("Resize", &state->show_resize_window);
    
    // Create aliases because the full expression to set the width and height in resize_info is very long
    int& width = state->image_action_info.resize_info.width;
    int& height = state->image_action_info.resize_info.height;
    
    // Create input text boxes
    ImGui::InputInt("New Width", &width, 0, 0, 0);
    ImGui::InputInt("New Height", &height, 0, 0, 0);
    
    // Make sure size is at least 1x1
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    
    // "OK" button
    if (ImGui::Button("OK")) {
        // Let backend know that we want to resize the canvas
        state->image_action_info.status = ImageActionInfo::DoResize;
        
        // Hide this window when "OK" is clicked
        state->show_resize_window = false;
    }
    
    // Create "Cancel" button on same line as "OK" button
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        // Hide window without performing resize action
        state->show_resize_window = false;
    }
    
    // End of resize window
    ImGui::End();
}

// Draw the "New File" window if user selects File->New in the menu bar
// Most of this is pretty similar to the resize window dialog
void drawNewFileWindow(State* state) {
    // Exit early if window is hidden
    if (!state->show_new_file_window) {
        return;
    }
    
    // Let ImGui determine best window size based on contents
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    
    // Start of window
    ImGui::Begin("New", &state->show_new_file_window);
    
    // Variable aliases
    int& width = state->file_action_info.new_info.width;
    int& height = state->file_action_info.new_info.height;
    
    // Input text boxes
    ImGui::InputInt("Width", &width, 0, 0, 0);
    ImGui::InputInt("Height", &height, 0, 0, 0);
    
    // Make sure size is at least 1x1
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    
    // "OK" button
    if (ImGui::Button("OK")) {
        // Let backend know that we want to create a new file
        state->file_action_info.status = FileActionInfo::DoNew;
        state->show_new_file_window = false;
    }
    
    // Create "Cancel" button on same line as "OK" button
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        state->show_new_file_window = false;
    }
    
    // End of resize window
    ImGui::End();
}

// Main entry point for drawing the GUI
// Does not do any rendering or presenting to the screen
void guiDraw(State* state) {
    // Start the ImGui frame which keeps track of submitted commands
    // Called once per frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    // This function is placed after the start of a new frame since some of ImGui's input utility functions don't work
    // if not inside a frame e.g. scroll wheel values
    guiUpdateStateMeta(state);
    
    // Draw various windows
    drawMainMenuBar(state);
    drawResizeWindow(state);
    drawNewFileWindow(state);
    
    // Menu bar
    {
        static float f = 0.0f;
        static int counter = 0;
        ImGui::SetNextWindowPos(ImVec2(state->window_width, 0), 0, ImVec2(1, 0));
        ImGui::SetNextWindowSize(ImVec2(state->right_menu_width, state->window_height));
        
        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::Checkbox("Want Capture?", &state->gui_resource->io->WantCaptureMouse);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorPicker3("clear color", (float*)&state->clear_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel ); // Edit 3 floats representing a color
        ImGui::ColorPicker4("Draw color", (float*)&state->draw_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel ); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / state->framerate, state->framerate);
        ImGui::Text("Size: %f", ImGui::GetWindowWidth());
        
        state->viewport.z = ImGui::GetWindowPos().x;
        
        ImGui::End();
    }
}

// Renders and presents the GUI and canvas to the screen
void guiPresent(State* state) {
    // Marks end of frame and finalizes submitted commands - does not actually draw the GUI to the window yet
    ImGui::Render();
    
    // The window is redrawn every frame, so it first needs to be cleared of any content from the last frame.
    // Set the draw color and then fill the window with that color
    SDL_SetRenderDrawColorFloat(state->gui_resource->renderer, state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    SDL_RenderClear(state->gui_resource->renderer);
    
    // Calculate the placement of the canvas on the screen by converting the top-left and bottom-right corners of the canvas from canvas-space to screen-space
    ImVec2 canvas_size{(float)state->canvas.width(), (float)state->canvas.height()};
    ImVec2 canvas_dest_tl = canvasToScreenPos(canvas_size, state->viewport, state->viewport_offset, state->scale, {0, 0});
    ImVec2 canvas_dest_br = canvasToScreenPos(canvas_size, state->viewport, state->viewport_offset, state->scale, canvas_size);

    // Finalize the destination rect by converting from x1,y2,x2,y2 format to x,y,w,h
    SDL_FRect canvas_dest_rect{canvas_dest_tl.x, canvas_dest_tl.y, canvas_dest_br.x - canvas_dest_tl.x, canvas_dest_br.y - canvas_dest_tl.y};

    // Render the canvas to the screen
    SDL_RenderTexture(state->gui_resource->renderer, state->canvas.get(), NULL, &canvas_dest_rect);
    
    // Render the GUI on top of the canvas
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), state->gui_resource->renderer);
    
    // Refreshes the screen with all the rendering since the last frame
    SDL_RenderPresent(state->gui_resource->renderer);
}