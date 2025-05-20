#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <memory>

class Texture {
public:
    Texture() {}
    Texture(SDL_Renderer* renderer, SDL_TextureAccess access, int w, int h);
    Texture(SDL_Renderer* renderer, SDL_Surface* surface);
    
    void fill(ImVec4 color);
    void drawLine(ImVec2 start, ImVec2 end, ImVec4 color);
    void loadFromArray(unsigned char* data); // Size is assumed to be w*h*3
    void renderTo(Texture dest, const SDL_FRect* src_rect, const SDL_FRect* dest_rect);
    void stampTextureAlongLine(Texture src, ImVec2 start, ImVec2 end);
    void setRenderTarget();
    
    int width() { return texture->w; }
    int height() { return texture->h; }
    ImVec2 size() { return {(float)width(), (float)height()}; }
    SDL_Texture* get() { return texture.get(); }
private:
    SDL_Renderer* renderer;
    SDL_TextureAccess access;
    std::shared_ptr<SDL_Texture> texture;
};