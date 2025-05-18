#include "backend.hpp"

#include <string>
#include <stdexcept>

void editPixel(SDL_Texture* texture, int x, int y, unsigned char rgba[4]) {
    unsigned char* texture_bytes;
    int pitch;
    SDL_LockTexture(texture, NULL, (void**)&texture_bytes, &pitch);

    unsigned char* pixel_dest = &texture_bytes[y * pitch + x * 4];
    memcpy(pixel_dest, rgba, 4);

    SDL_UnlockTexture(texture);
}

void backendInit(State* state) {
    state->clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    // Image
    state->canvas = SDL_CreateTexture(state->gui_resource->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 256);
    if (state->canvas == nullptr)
        throw std::runtime_error(std::string("Error: SDL_CreateTexture(): ") + SDL_GetError());
    
    unsigned char* texture_bytes;
    int pitch;
    SDL_LockTexture(state->canvas, NULL, (void**)&texture_bytes, &pitch);
    unsigned char rgba[4] = {255, 0, 0, 0};
    for (int y = 0; y < 256; y++) {
        for (int x = 0; x < 256; x++) {
            unsigned char* pixel_dest = &texture_bytes[y * pitch + x * sizeof(rgba)];
            rgba[3] = x;
            rgba[1] = y;
            memcpy(pixel_dest, rgba, sizeof(rgba));
        }
    }
    SDL_UnlockTexture(state->canvas);
}


void backendProcess(State* state) {
    if (state->mouse_down) {
            if (state->mouse_x >= 0 && state->mouse_x < 256) {
                if (state->mouse_y >= 0 && state->mouse_y < 256) {
                    unsigned char rgba[4] = {255, 255, 255, 255};
                    editPixel(state->canvas, state->mouse_x, state->mouse_y, rgba);
                }
            }
        }
}