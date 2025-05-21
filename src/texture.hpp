#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <memory>

class Texture {
public:
    // Default constructor, does not create texture
    Texture() {}
    
    // Texture constructor for new blank texture given width and height
    Texture(SDL_Renderer* renderer, SDL_TextureAccess access, int w, int h);
    
    // Texture constructor for new texture created from an existing surface
    Texture(SDL_Renderer* renderer, SDL_Surface* surface);
    
    // Fill texture with solid color
    void fill(ImVec4 color);
    
    // Draw a one-pixel wide line from the start to the end position
    void drawLine(ImVec2 start, ImVec2 end, ImVec4 color);
    
    // Create a new texture from an array
    void loadFromArray(unsigned char* data); // Size is assumed to be w*h*4 (4 bytes per pixel)
    
    // Render this texture to another one
    void renderTo(Texture dest, const SDL_FRect* src_rect, const SDL_FRect* dest_rect);
    
    // Renders a provided texture to every point of this texture that is along a line
    void stampTextureAlongLine(Texture src, ImVec2 start, ImVec2 end);
    
    // Set this texture as the render target
    void setRenderTarget();
    
    // Getters for width, height, size, and underlying texture pointer
    int width() { return texture->w; }
    int height() { return texture->h; }
    ImVec2 size() { return {(float)width(), (float)height()}; }
    SDL_Texture* get() { return texture.get(); }

private:
    SDL_Renderer* renderer;
    SDL_TextureAccess access;
    std::shared_ptr<SDL_Texture> texture;
};