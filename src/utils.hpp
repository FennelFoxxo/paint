#pragma once

#include <imgui.h>
#include <SDL3/SDL.h>
#include <vector>
#include <tuple>

static ImVec2 canvasToScreenPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point) {
    // Scale everything up
    point.x *= scale;
    point.y *= scale;
    
    canvas_size.x *= scale;
    canvas_size.y *= scale;
    viewport_offset.x *= scale;
    viewport_offset.y *= scale;
    
    point.x += viewport.x + viewport.z / 2.0f - canvas_size.x / 2.0f - viewport_offset.x;
    point.y += viewport.y + viewport.w / 2.0f - canvas_size.y / 2.0f - viewport_offset.y;
    
    return point;
}

static ImVec2 screenToCanvasPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point) {
    canvas_size.x *= scale;
    canvas_size.y *= scale;
    viewport_offset.x *= scale;
    viewport_offset.y *= scale;
    
    point.x -= viewport.x + viewport.z / 2.0f - canvas_size.x / 2.0f - viewport_offset.x;
    point.y -= viewport.y + viewport.w / 2.0f - canvas_size.y / 2.0f - viewport_offset.y;
    
    point.x /= scale;
    point.y /= scale;
    
    return point;
}

static ImVec4 scaleVec(ImVec4 in, float scale) {
    return {in.x * scale, in.y * scale, in.z * scale, in.w * scale};
}

static Uint32 vecToUint32(SDL_PixelFormat format, ImVec4 in) {
    return SDL_MapRGBA(SDL_GetPixelFormatDetails(format), nullptr, in.x, in.y, in.z, in.w);
}

static Uint32* getPixel(void* array, int pitch, int x, int y) {
    return &((Uint32*)array)[y * pitch / sizeof(Uint32) + x];
}

static void editPixel(void* array, int pitch, int x, int y, Uint32 rgba) {
    *getPixel(array, pitch, x, y) = rgba;
}

static void drawCircle(SDL_Surface* surface, int radius, ImVec4 color) {
    int diameter = radius * 2;

    int center_x = surface->w / 2;
    int center_y = surface->h / 2;
    
    unsigned char* pixels = (unsigned char*)surface->pixels;
    int pitch = surface->pitch;

    Uint32 rgba = vecToUint32(surface->format, scaleVec(color, 255));

    int x = radius - 1;
    int y = 0;
    int tx = 1;
    int ty = 1;
    int error = tx - diameter;

    while (x >= y) {
        editPixel(pixels, pitch, center_x + x, center_y + y, rgba);
        editPixel(pixels, pitch, center_x + x, center_y - y, rgba);
        editPixel(pixels, pitch, center_x - x, center_y + y, rgba);
        editPixel(pixels, pitch, center_x - x, center_y - y, rgba);
        
        editPixel(pixels, pitch, center_x + y, center_y + x, rgba);
        editPixel(pixels, pitch, center_x + y, center_y - x, rgba);
        editPixel(pixels, pitch, center_x - y, center_y + x, rgba);
        editPixel(pixels, pitch, center_x - y, center_y - x, rgba);
        
        if (error <= 0) {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0) {
            --x;
            tx += 2;
            error += tx - diameter;
        }
    }
}

static void floodFill(SDL_Surface* surface, ImVec2 pos, ImVec4 draw_color_vec) {
    Uint32 draw_color = vecToUint32(surface->format, scaleVec(draw_color_vec, 255));
    
    Uint32 starting_color = *getPixel(surface->pixels, surface->pitch, pos.x, pos.y); 
    
    std::vector<std::tuple<int, int>> stack;
    stack.push_back({pos.x, pos.y});
    
    while (!stack.empty()) {
        auto [this_x, this_y] = stack[stack.size() - 1];
        stack.pop_back();
        
        if (this_x < 0 || this_x >= surface->w || this_y < 0 || this_y >= surface->h) continue;
        
        Uint32* this_pixel = getPixel(surface->pixels, surface->pitch, this_x, this_y);

        if (*this_pixel != starting_color || *this_pixel == draw_color) continue;
        
        *this_pixel = draw_color;
        
        stack.push_back({this_x+1, this_y});
        stack.push_back({this_x-1, this_y});
        stack.push_back({this_x, this_y+1});
        stack.push_back({this_x, this_y-1});
    }
    
}