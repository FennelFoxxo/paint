#include "backend.hpp"
#include "texture.hpp"
#include "utils.hpp"

#include <string>
#include <stdexcept>
#include <cmath>
#include <iostream>

void editPixel(State* state, SDL_Texture* texture, int x, int y, unsigned char rgba_src[4]) {
    Texture temp(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, 1, 1);
    temp.fill(rgba_src[3], rgba_src[2], rgba_src[1]);
    temp.renderTo(state->canvas, nullptr, &(const SDL_FRect&){(float)x, (float)y, 1, 1});
}

void recreateCanvas(State* state, int width, int height) {
    state->canvas = Texture(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, width, height);
    state->canvas.fill(255, 255, 255);
}

void resizeCanvas(State* state, int new_width, int new_height) {
    Texture new_canvas(state->gui_resource->renderer, SDL_TEXTUREACCESS_TARGET, new_width, new_height);
    state->canvas.renderTo(new_canvas, nullptr, nullptr);
    state->canvas = new_canvas;
}

void backendInit(State* state) {
    state->clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    // Image
    recreateCanvas(state, 400, 400);
}

void handleDraw(State* state) {
    if (!state->mouse_left_down) {
        return;
    }
    if (!state->mouse_over_canvas) {
        return;
    }
    
    ImVec2 canvas_size{(float)state->canvas.width(), (float)state->canvas.height()};
    ImVec2 canvas_pos = screenToCanvasPos(canvas_size, state->viewport, state->viewport_offset, state->scale, state->mouse_pos);
    ImVec2 canvas_pos_old = screenToCanvasPos(canvas_size, state->viewport, state->viewport_offset, state->scale, state->mouse_pos_old);

    SDL_SetRenderDrawColorFloat(state->gui_resource->renderer, state->draw_color.x, state->draw_color.y, state->draw_color.z, state->draw_color.w);
    SDL_SetRenderTarget(state->gui_resource->renderer, state->canvas.get());
    SDL_RenderLine(state->gui_resource->renderer, canvas_pos_old.x, canvas_pos_old.y, canvas_pos.x, canvas_pos.y);
    SDL_SetRenderTarget(state->gui_resource->renderer, nullptr);
}

void handleNewFile(State* state) {
    if (state->file_action_info.status == FileActionInfo::DoNew) {
        int new_width = state->file_action_info.new_info.width;
        int new_height = state->file_action_info.new_info.height;
        recreateCanvas(state, new_width, new_height);
        state->file_action_info.status = FileActionInfo::None;
    }
}

void handleImageResize(State* state) {
    if (state->image_action_info.status == ImageActionInfo::DoResize) {
        int width = state->image_action_info.resize_info.width;
        int height = state->image_action_info.resize_info.height;
        resizeCanvas(state, width, height);
        state->image_action_info.status = ImageActionInfo::None;
    }
}

void handleScroll(State* state) {
    state->scale *= pow(1.1, state->scroll);
    if (state->scale < 0.1) state->scale = 0.1;
    if (state->scale > 10) state->scale = 10;
    
}

void handleRightDrag(State* state) {
    if (state->mouse_right_dragging) {
        if (state->canvas_dragging_state == CanvasDraggingState::not_dragging) {
            state->canvas_dragging_state = state->mouse_over_canvas ? CanvasDraggingState::dragging_canvas : CanvasDraggingState::dragging_off_canvas;
        }
    } else {
        state->canvas_dragging_state = CanvasDraggingState::not_dragging;
    }
    
    if (state->canvas_dragging_state == CanvasDraggingState::dragging_canvas) {
        state->viewport_offset.x -= (state->drag_delta.x - state->drag_delta_old.x) / state->scale;
        state->viewport_offset.y -= (state->drag_delta.y - state->drag_delta_old.y) / state->scale;
    }
    
    state->drag_delta_old = state->drag_delta;
}

void backendProcess(State* state) {
    handleDraw(state);
    handleNewFile(state);
    handleImageResize(state);
    handleRightDrag(state);
    handleScroll(state);
    state->mouse_pos_old = state->mouse_pos;
}