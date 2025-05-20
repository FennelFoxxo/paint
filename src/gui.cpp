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
    state->mouse_pos.screen = ImGui::GetMousePos(); // xy mouse pos on window
    state->mouse_pos.updateCanvasPos(state);
    
    state->scroll = state->gui_resource->io->MouseWheel; // Mouse scroll wheen input - positive for scrolling up, negative for down
    
    // Left mouse button
    state->lmb_info.down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (state->lmb_info.down && !state->lmb_info_old.down)
        state->lmb_info.drag_start = state->mouse_pos;
    
    // Right mouse button
    state->rmb_info.down = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if (state->rmb_info.down && !state->rmb_info_old.down)
        state->rmb_info.drag_start = state->mouse_pos;
    
    // True if the mouse is over a GUI element or if the mouse started dragging over a GUI element
    state->gui_wants_mouse = state->gui_resource->io->WantCaptureMouse;
    
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
                state->file_action_info.status = FileActionInfo::DoOpen;
            }
            
            // "Save As" button
            if (ImGui::MenuItem("Save As")) {
                state->file_action_info.status = FileActionInfo::DoSaveAs;
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
    float& width_f = state->image_action_info.resize_info.size.x;
    float& height_f = state->image_action_info.resize_info.size.y;
    
    // The text box should accept an int but it needs to be converted to a float
    int width_i = width_f, height_i = height_f;
    
    // Create input text boxes
    ImGui::InputInt("New Width", &width_i, 0, 0, 0);
    ImGui::InputInt("New Height", &height_i, 0, 0, 0);

    // Make sure size is at least 1x1
    if (width_i < 1) width_i = 1;
    if (height_i < 1) height_i = 1;
    
    width_f = width_i, height_f = height_i;
    
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
    float& width_f = state->file_action_info.new_info.size.x;
    float& height_f = state->file_action_info.new_info.size.y;
    
    // The text box should accept an int but it needs to be converted to a float
    int width_i = width_f, height_i = height_f;
    
    // Input text boxes
    ImGui::InputInt("Width", &width_i, 0, 0, 0);
    ImGui::InputInt("Height", &height_i, 0, 0, 0);
    
    // Make sure size is at least 1x1
    if (width_i < 1) width_i = 1;
    if (height_i < 1) height_i = 1;
    
    width_f = width_i, height_f = height_i;
    
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


void drawRightMenu(State* state) {
    // Set window position so that the right edge is aligned with the window,
    // and the top edge is aligned with the bottom of the menu bar
    ImGui::SetNextWindowPos(ImVec2(state->window_width, state->viewport.y), 0, ImVec2(1, 0));
    
    // Force width to be preset amount and height to be full height of window
    ImGui::SetNextWindowSize(ImVec2(state->right_menu_width, state->window_height));
    
    // Create a window called "Hello, world!" and append into it.
    ImGui::Begin("Hello, world!", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Remove padding just for the drawing tool icon buttons
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    
    // Temp variable to keep track of the background color for each icon
    ImVec4 this_icon_color;
    
    // Use "selected" color if brush tool is set
    this_icon_color = state->drawing_tool == DrawingTool::Brush ? state->selected_icon_color : state->unselected_icon_color;
    
    // Create brush tool button
    if (ImGui::ImageButton("Brush", (ImTextureID)state->icons.brush.get(), state->icons.brush.size(), {0, 0}, ImVec2(1, 1), this_icon_color)) {
        // Use brush tool if button is clicked
        state->drawing_tool = DrawingTool::Brush;
    }
    
    // Use "selected" color if line tool is set
    this_icon_color = state->drawing_tool == DrawingTool::Line ? state->selected_icon_color : state->unselected_icon_color;
    
    // Create line tool button on same line
    ImGui::SameLine();
    if (ImGui::ImageButton("Line", (ImTextureID)state->icons.line.get(), state->icons.line.size(), {0, 0}, ImVec2(1, 1), this_icon_color)) {
        // Use line tool if button is clicked
        state->drawing_tool = DrawingTool::Line;
    }
    
    // Use "selected" color if fill tool is set
    this_icon_color = state->drawing_tool == DrawingTool::Fill ? state->selected_icon_color : state->unselected_icon_color;
    
    // Create fill tool button on same line
    ImGui::SameLine();
    if (ImGui::ImageButton("Fill", (ImTextureID)state->icons.fill.get(), state->icons.fill.size(), {0, 0}, ImVec2(1, 1), this_icon_color)) {
        // Use fill tool if button is clicked
        state->drawing_tool = DrawingTool::Fill;
    }
    
    // End of buttons, restore original style
    ImGui::PopStyleVar();
    
    ImGui::Text("Brush size");
    if(ImGui::SliderInt("##Brush size", &state->brush_size, 1, 100)) {
        state->brush_details_changed = true;
    }

    // Edit 3 floats representing a color
    if (ImGui::ColorPicker3("Draw color", (float*)&state->draw_color, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoLabel)) {
        state->brush_details_changed = true;
    }

    ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / state->framerate, state->framerate);
    ImGui::Text("Size: %f", ImGui::GetWindowWidth());
    
    state->viewport.z = ImGui::GetWindowPos().x;
    
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
    drawRightMenu(state);
}

// Renders and presents the GUI and canvas to the screen
void guiPresent(State* state) {
    // Alias
    SDL_Renderer* renderer = state->gui_resource->renderer;
    
    // Make sure we're rendering to the window instead of potentially rendering to a texture that was set as the target
    SDL_SetRenderTarget(renderer, nullptr);
    
    // Marks end of frame and finalizes submitted commands - does not actually draw the GUI to the window yet
    ImGui::Render();
    
    // The window is redrawn every frame, so it first needs to be cleared of any content from the last frame.
    // Set the draw color and then fill the window with that color
    SDL_SetRenderDrawColorFloat(renderer, state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    SDL_RenderClear(renderer);
    
    // Canvas rendering
    {
        // Calculate the placement of the canvas on the screen by converting the top-left and bottom-right corners of the canvas from canvas-space to screen-space
        ImVec2 canvas_dest_tl = canvasToScreenPos(state->canvas.size(), state->viewport, state->viewport_offset, state->scale, {0, 0});
        ImVec2 canvas_dest_br = canvasToScreenPos(state->canvas.size(), state->viewport, state->viewport_offset, state->scale, state->canvas.size());

        // Finalize the destination rect by converting from x1,y1,x2,y2 format to x,y,w,h
        SDL_FRect canvas_dest_rect{canvas_dest_tl.x, canvas_dest_tl.y, canvas_dest_br.x - canvas_dest_tl.x, canvas_dest_br.y - canvas_dest_tl.y};

        // Render the canvas to the screen
        SDL_RenderTexture(renderer, state->canvas.get(), NULL, &canvas_dest_rect);
    }
    
    // Brush tool preview rendering
    if (state->drawing_tool == DrawingTool::Brush || state->drawing_tool == DrawingTool::Line) {
        ImVec2 brush_preview_size = state->brush_texture_preview.size();
        
        SDL_FRect brush_dest_rect{
            state->mouse_pos.screen.x - brush_preview_size.x / 2 * state->scale,
            state->mouse_pos.screen.y - brush_preview_size.y / 2 * state->scale,
            brush_preview_size.x * state->scale,
            brush_preview_size.y * state->scale
        };
        SDL_RenderTexture(renderer, state->brush_texture_preview.get(), NULL, &brush_dest_rect);
    }
    
    // Show preview of line if the user is currently drawing a line
    // Draw it after the canvas to make sure it's on top
    if (state->drawing_line) {
        // Set draw color
        SDL_SetRenderDrawColorFloat(renderer, state->draw_color.x, state->draw_color.y, state->draw_color.z, state->draw_color.w);

        // Start screen position might have changed if user scrolled canvas while drawing line
        // Get original canvas position of start pos and then map that back into screen space
        ImVec2 start_canvas = state->draw_line_start.canvas;
        ImVec2 start_screen = canvasToScreenPos(state->canvas.size(), state->viewport, state->viewport_offset, state->scale, start_canvas);
        
        
        ImVec2 end = state->draw_line_end.screen;
        SDL_RenderLine(renderer, start_screen.x, start_screen.y, end.x, end.y);
    }
    
    // Render the GUI on top of the canvas
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    
    // Refreshes the screen with all the rendering since the last frame
    SDL_RenderPresent(renderer);
}