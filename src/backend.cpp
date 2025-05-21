#include "backend.hpp"
#include "texture.hpp"
#include "utils.hpp"

#include <string>
#include <stdexcept>
#include <cmath>

// When the canvas is created, resized, or loaded from an image, we should update the default
// "File->New" and "Image->Resize" options to the new canvas size just for QOL so the new resolution
// doesn't need to be retyped every time
void updateCanvasOptionValues(State* state) {
    state->file_action_info.new_info.size = state->canvas.size();
    state->image_action_info.resize_info.size = state->canvas.size();
}

// Set the canvas to a new blank white texture with given size, deleting the old texture if a canvas already exists
void recreateCanvas(State* state, ImVec2 size) {
    // Create new texture with target access mode so it can be rendered to
    // Implicitly deletes the old canvas when the existing state->canvas object goes out of scope
    state->canvas = Texture(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
    
    // Set scaling mode to nearest so pixels don't get blurry when you zoom in
    SDL_SetTextureScaleMode(state->canvas.get(), SDL_SCALEMODE_NEAREST);
    
    // Fill it with solid white
    state->canvas.fill({1.0f, 1.0f, 1.0f, 1.0f});
    
    // Update the default values with new canvas size
    updateCanvasOptionValues(state);
}

// Resize the canvas to the given size without erasing content
void resizeCanvas(State* state, ImVec2 size) {
    // Create a new blank texture with the desired canvas size
    Texture new_canvas(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
    
    // Set scaling mode to nearest so pixels don't get blurry when you zoom in
    SDL_SetTextureScaleMode(new_canvas.get(), SDL_SCALEMODE_NEAREST);
    
    // Render existing canvas to the new texture - SDL will stretch and scale it to fit
    state->canvas.renderTo(new_canvas, nullptr, nullptr);
    
    // Assign new texture as the canvas - implictly deletes old canvas
    state->canvas = new_canvas;
    
    // Update the default values with new canvas size
    updateCanvasOptionValues(state);
}

// The brush texture is a texture that is stamped on the canvas during a draw event
// SDL doesn't have a renderCicle function, so creating our own circle ourselves enables
// a brush size bigger than just a single pixel
// However, the texture needs to be updated any time the brush size or color changes
void recreateBrushTexture(State* state, int radius, ImVec4 color) {
    // Make sure radius is at least 1
    radius = (radius == 0 ? 1 : radius);
    
    // Create new blank surface with twice the radius, just big enough to draw circle to
    SDL_Surface* surface = SDL_CreateSurface(radius*2, radius*2, SDL_PIXELFORMAT_RGBA8888);
    
    // Call util function to draw the outline of a circle
    drawCircle(surface, radius, color);
    
    // The preview texture (when hovering over the canvas) should just be the circle outline,
    // but the brush texture itself needs to be solid in order to fill a solid circle when stamping
    // it to the canvas
    // The current surface just has the outline of the circle which is what the preview needs to be,
    // so create a texture from the surface and set scale mode to nearest so it doesn't get blurry when zooming in
    state->brush_texture_preview = Texture(state->gui_resource->renderer, surface);
    SDL_SetTextureScaleMode(state->brush_texture_preview.get(), SDL_SCALEMODE_NEAREST);
    
    // Now we can fill in the texture by performing a flood fill with the draw color, starting at
    // the center of the circle
    floodFill(surface, {surface->w/2.0f, surface->h/2.0f}, state->draw_color);
    
    // Update brush texture with new filled-in surface
    state->brush_texture = Texture(state->gui_resource->renderer, surface);
    
    // Destroy temporary surface
    SDL_DestroySurface(surface);
}

// Initializes the state and creates some required objects e.g. canvas and icon textures
void backendInit(State* state) {
    // Temporary surface for loading icon textures
    SDL_Surface* temp_surface;

    // Load brush, line, and bucket icons
    temp_surface = openImage("icons/brush.png");
    state->icons.brush = Texture(state->gui_resource->renderer, temp_surface);
    
    temp_surface = openImage("icons/line.png");
    state->icons.line = Texture(state->gui_resource->renderer, temp_surface);
    
    temp_surface = openImage("icons/bucket.png");
    state->icons.fill = Texture(state->gui_resource->renderer, temp_surface);
    
    // Create initial brush texture
    // Brush size is the width of the brush, so the radius is size/2
    recreateBrushTexture(state, state->brush_size / 2, state->draw_color);
    
    // Create initial blank canvas
    recreateCanvas(state, state->initial_canvas_size);
}

// Process drawing with the brush tool
void handleDrawBrush(State* state) {
    // Return early if mouse is not held down
    if (!state->lmb_info.down) return;
    
    // Return early if the mouse is not over the canvas
    if (state->gui_wants_mouse) return;

    // If the user moves the mouse quickly, there might be large gaps between the reported mouse position
    // So, we need to draw a solid line between the last position and the current position to make the drawing continuous
    state->canvas.stampTextureAlongLine(state->brush_texture, state->mouse_pos_old.canvas, state->mouse_pos.canvas);
}

// Process drawing with the line tool
void handleDrawLine(State* state){
    // Return early if the mouse is not over the canvas
    if (state->gui_wants_mouse) return;
    
    // If the user just started dragging the mouse
    if (state->lmb_info.down && !state->lmb_info_old.down) {
        // Track start position
        state->draw_line_start = state->mouse_pos;
        state->drawing_line = true;
    }
    
    // If the user is currently dragging the mouse, continually update the end position so the line preview can be displayed
    if (state->lmb_info.down) {
        state->draw_line_end = state->mouse_pos;
    }
    
    // If the user just let go of the mouse and we're currently drawing a line
    if (!state->lmb_info.down && state->lmb_info_old.down && state->drawing_line) {
        // Draw line from start to end position
        state->canvas.stampTextureAlongLine(state->brush_texture, state->draw_line_start.canvas, state->draw_line_end.canvas);
        state->drawing_line = false;
    }
}

// Process drawing with the fill tool
void handleDrawFill(State* state) {
    // Return early if the mouse is over GUI elements
    if (state->gui_wants_mouse) return;
    
    // Only need to fill if user just clicked the mouse
    if (!state->lmb_info.down || state->lmb_info_old.down) return;
    
    // Make sure mouse cursor is over the canvas
    if (state->mouse_pos.canvas.x < 0 || state->mouse_pos.canvas.x > state->canvas.width() ||
        state->mouse_pos.canvas.y < 0 || state->mouse_pos.canvas.y > state->canvas.height()) return;

    // Create a surface from the canvas texture so we can modify its pixels directly
    state->canvas.setRenderTarget();
    SDL_Surface* canvas_surface = SDL_RenderReadPixels(state->gui_resource->renderer, NULL);

    // Fill the surface with the draw color at the mouse position
    floodFill(canvas_surface, state->mouse_pos.canvas, state->draw_color);
    
    // Create a new texture from the result of the filled surface
    // We can't directly set the canvas to be this texture, because the canvas needs to have target access mode
    // for rendering to it but creating a texture from a surface forces it to have streaming access mode
    Texture filled_canvas = Texture(state->gui_resource->renderer, canvas_surface);
    
    // Create new blank canvas with same size as current canvas
    recreateCanvas(state, state->canvas.size());
    
    // Copy the data of the filled canvas to the newly created canvas
    filled_canvas.renderTo(state->canvas, NULL, NULL);
    
    // Clean up surface
    SDL_DestroySurface(canvas_surface);
}

// Process drawing on canvas
void handleDraw(State* state) {
    // Switch depending on current selected tool
    switch(state->drawing_tool) {
        case DrawingTool::Brush:
            handleDrawBrush(state);
            break;
        case DrawingTool::Line:
            handleDrawLine(state);
            break;
        case DrawingTool::Fill:
            handleDrawFill(state);
            break;
    }
}

// Called if the user selects "File->New" in the top menu bar
void handleNewFile(State* state) {
    // Create a new canvas with user-selected size
    recreateCanvas(state, state->file_action_info.new_info.size);
}

// Called if the user selects "File->Open" in the top menu bar
void handleOpenFile(State* state) {
    // Open file dialog asking user where to save file
    std::string path = requestFileDialog({ { "PNG", "png" }, { "JPG", "jpg" } }, false);
    
    // Return if path is empty (user cancelled)
    if (path.empty()) return;
    
    // Read image file from path
    SDL_Surface* image_surface = openImage(path);
    
    // Create texture from surface of image
    Texture image_texture = Texture(state->gui_resource->renderer, image_surface);
    
    // Create new blank canvas with same size as the opened image
    recreateCanvas(state, image_texture.size());
    
    // Copy the data of the image to the newly created canvas
    image_texture.renderTo(state->canvas, NULL, NULL);
    
    // Clean up surface
    SDL_DestroySurface(image_surface);
}

// Called if the user selects "File->Save As" in the top menu bar
void handleSaveAsFile(State* state) {
    // Open file dialog asking user where to save file
    std::string path = requestFileDialog({ { "PNG", "png" }, { "JPG", "jpg" } }, true);
    
    // Return if path is empty (user cancelled)
    if (path.empty()) return;
    
    // Create a surface from the canvas texture so we can read its pixels directly
    state->canvas.setRenderTarget();
    SDL_Surface* canvas_surface = SDL_RenderReadPixels(state->gui_resource->renderer, NULL);
    
    // Write image file to path
    saveImage(path, canvas_surface);
    
    // Clean up surface
    SDL_DestroySurface(canvas_surface);
}

// Called if the user selects "Image->Resize" in the top menu bar
void handleImageResize(State* state) {
    // Resize canvas to user-selected size
    resizeCanvas(state, state->image_action_info.resize_info.size);
}

// Process any actions caused by the user clicking an option in the top menu bar e.g. File->New
void handleMenuBarAction(State* state) {
    // Dispatch actions if the user clicked an option in the File menu
    switch (state->file_action_info.status) {
        case FileActionInfo::DoNew:
            handleNewFile(state);
            break;
        case FileActionInfo::DoOpen:
            handleOpenFile(state);
            break;
        case FileActionInfo::DoSaveAs:
            handleSaveAsFile(state);
            break;
        default:
            break;
    }
    // Action has been processed, clear status
    state->file_action_info.status = FileActionInfo::None;
    
    // Dispatch actions if the user clicked an option in the Image menu
    switch (state->image_action_info.status) {
        case ImageActionInfo::DoResize:
            handleImageResize(state);
            break;
        default:
            break;
    }
    // Action has been processed, clear status
    state->image_action_info.status = ImageActionInfo::None;
}

// Handle any scroll wheel inputs
void handleScroll(State* state) {
    // Every scroll wheel input should multiply or divide the scale by a fixed amount
    // If the user is scrolling quickly, state->scroll might be large so we need to multiply or divide repeatedly (which is what pow does)
    state->scale *= pow(1.1, state->scroll);
    
    // Cap scale between 0.1 and 10
    if (state->scale < 0.1) state->scale = 0.1;
    if (state->scale > 10) state->scale = 10;
    
}

// Handle dragging the canvas if the user drags with RMB
void handleCanvasDrag(State* state) {
    // Alias
    ImVec2 mouse = state->mouse_pos.screen;
    ImVec2 mouse_old = state->mouse_pos_old.screen;
    
    // Is the RMB being used to drag the mouse and is the mouse on the canvas?
    if (state->rmb_info.down && !state->gui_wants_mouse) {
        // Adjust viewport offset by the amount the mouse moved since the last frame
        // Divide by scale because the canvas should drag slower if very zoomed in
        state->viewport_offset.x -= (mouse.x - mouse_old.x) / state->scale;
        state->viewport_offset.y -= (mouse.y - mouse_old.y) / state->scale;
    }
}

// Handle updating the brush texture (and brush preview texture) if any details (e.g. brush size or color) changed
void handleBrushDetailsChange(State* state) {
    if (state->brush_details_changed) {
        state->brush_details_changed = false;
        // Brush size is the width of the brush, so the radius is size/2
        recreateBrushTexture(state, state->brush_size / 2, state->draw_color);
    }
}

// Update any _old variables so the values of variables can be compared between two frames
// Should be called at the end of backend processing
void updateOldVars(State* state) {
    state->lmb_info_old = state->lmb_info;
    state->rmb_info_old = state->rmb_info;
    
    state->mouse_pos_old = state->mouse_pos;
}

// Process events that happened e.g. if user dragged mouse to draw
void backendProcess(State* state) {
    handleDraw(state);
    handleMenuBarAction(state);
    handleCanvasDrag(state);
    handleScroll(state);
    handleBrushDetailsChange(state);
    
    updateOldVars(state);
}