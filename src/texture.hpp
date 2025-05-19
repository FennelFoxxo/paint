#pragma once

#include <SDL3/SDL.h>
#include <memory>

class Texture {
public:
    Texture() {}
    Texture(SDL_Renderer* renderer, SDL_TextureAccess access, int w, int h);
    
    void fill(unsigned char r, unsigned char g, unsigned char b);
    void loadFromArray(unsigned char* data); // Size is assumed to be w*h*3
    void renderTo(Texture& dest, const SDL_FRect* src_rect, const SDL_FRect* dest_rect);
    
    int width() { return texture->w; }
    int height() { return texture->h; }
    SDL_Texture* get() { return texture.get(); }
private:
    SDL_Renderer* renderer;
    SDL_TextureAccess access;
    std::shared_ptr<SDL_Texture> texture;
};