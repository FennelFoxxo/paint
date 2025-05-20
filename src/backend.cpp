#include "backend.hpp"
#include "texture.hpp"
#include "utils.hpp"

#include <string>
#include <stdexcept>
#include <cmath>
#include <iostream>

// When the canvas is created, resized, or loaded from an image, we should update the default
// "File->New" and "Image->Resize" options to the new canvas size just for QOL so the new resolution
// doesn't need to be retyped every time
void updateCanvasOptionValues(State* state) {
    state->file_action_info.new_info.size = state->canvas.size();
    state->image_action_info.resize_info.size = state->canvas.size();
}


void recreateCanvas(State* state, ImVec2 size) {
    state->canvas = Texture(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
    SDL_SetTextureScaleMode(state->canvas.get(), SDL_SCALEMODE_NEAREST);
    
    state->canvas.fill({1.0f, 1.0f, 1.0f, 1.0f});
    updateCanvasOptionValues(state);
}

void resizeCanvas(State* state, ImVec2 size) {
    Texture new_canvas(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
    SDL_SetTextureScaleMode(new_canvas.get(), SDL_SCALEMODE_NEAREST);
    
    state->canvas.renderTo(new_canvas, nullptr, nullptr);
    state->canvas = new_canvas;
    updateCanvasOptionValues(state);
}

void recreateBrushTexture(State* state, int radius, ImVec4 color) {
    // Make sure radius is at least 1
    radius = (radius == 0 ? 1 : radius);
    
    SDL_Surface* surface = SDL_CreateSurface(radius*2, radius*2, SDL_PIXELFORMAT_RGBA8888);
    drawCircle(surface, radius, color);
    
    state->brush_texture_preview = Texture(state->gui_resource->renderer, surface);
    SDL_SetTextureScaleMode(state->brush_texture_preview.get(), SDL_SCALEMODE_NEAREST);
    
    floodFill(surface, {surface->w/2.0f, surface->h/2.0f}, state->draw_color);
    state->brush_texture = Texture(state->gui_resource->renderer, surface);
    
    SDL_DestroySurface(surface);
}

void backendInit(State* state) {
    state->icons.brush = Texture(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, 48, 48);
    state->icons.brush.fill({1.0f, 1.0f, 0.0f, 0.5f});
    
    state->icons.line = Texture(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, 48, 48);
    state->icons.line.fill({1.0f, 1.0f, 0.0f, 0.5f});
    
    state->icons.fill = Texture(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, 48, 48);
    state->icons.fill.fill({1.0f, 1.0f, 0.0f, 0.5f});
    
    recreateBrushTexture(state, state->brush_size / 2, state->draw_color);
    recreateCanvas(state, state->initial_canvas_size);
}

// Process drawing with the brush tool
void handleDrawBrush(State* state) {
    // Return early if mouse is not held down
    if (!state->lmb_info.down) return;
    
    // Return early if the mouse is not over the canvas
    if (state->gui_wants_mouse) return;

    // Draw line
    state->canvas.stampTextureAlongLine(state->brush_texture, state->mouse_pos_old.canvas, state->mouse_pos.canvas);
}

// Process drawing with the line tool
void handleDrawLine(State* state){
    // Return early if the mouse is not over the canvas
    if (state->gui_wants_mouse) return;
    
    // If the user just started dragging the mouse
    if (state->lmb_info.down && !state->lmb_info_old.down) {
        state->draw_line_start = state->mouse_pos;
        state->drawing_line = true;
    }
    
    // If the user is currently dragging the mouse, continually update the end position so the line preview can be displayed
    if (state->lmb_info.down) {
        state->draw_line_end = state->mouse_pos;
    }
    
    // If the user just let go of the mouse
    if (!state->lmb_info.down && state->lmb_info_old.down && state->drawing_line) {
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
    // We can't directly copy this to the canvas, because the canvas needs to have target access mode for rendering to it,
    // but creating a texture from a surface forces it to have streaming access mode
    Texture filled_canvas = Texture(state->gui_resource->renderer, canvas_surface);
    
    // Create new blank canvas with same size as current canvas
    recreateCanvas(state, state->canvas.size());
    
    // Copy the data of the filled canvas to the newly created canvas
    filled_canvas.renderTo(state->canvas, NULL, NULL);
}

// Process drawing on canvas
void handleDraw(State* state) {
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

void handleNewFile(State* state) {
    if (state->file_action_info.status == FileActionInfo::DoNew) {
        recreateCanvas(state, state->file_action_info.new_info.size);
        state->file_action_info.status = FileActionInfo::None;
    }
}

void handleImageResize(State* state) {
    if (state->image_action_info.status == ImageActionInfo::DoResize) {
        resizeCanvas(state, state->image_action_info.resize_info.size);
        state->image_action_info.status = ImageActionInfo::None;
    }
}

void handleScroll(State* state) {
    state->scale *= pow(1.1, state->scroll);
    if (state->scale < 0.1) state->scale = 0.1;
    if (state->scale > 10) state->scale = 10;
    
}

void handleCanvasDrag(State* state) {
    // Alias
    ImVec2 mouse = state->mouse_pos.screen;
    ImVec2 mouse_old = state->mouse_pos_old.screen;
    
    // Is the RMB being used to drag the mouse and is the mouse on the canvas?
    if (state->rmb_info.down && !state->gui_wants_mouse) {
        state->viewport_offset.x -= (mouse.x - mouse_old.x) / state->scale;
        state->viewport_offset.y -= (mouse.y - mouse_old.y) / state->scale;
    }
}

void handleBrushDetailsChange(State* state) {
    if (state->brush_details_changed) {
        state->brush_details_changed = false;
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
    handleNewFile(state);
    handleImageResize(state);
    handleCanvasDrag(state);
    handleScroll(state);
    handleBrushDetailsChange(state);
    
    updateOldVars(state);
}