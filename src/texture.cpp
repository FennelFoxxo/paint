#include "texture.hpp"
#include "utils.hpp"

#include <string>
#include <stdexcept>
#include <cmath>

Texture::Texture(SDL_Renderer* renderer, SDL_TextureAccess access, int w, int h) {
    this->renderer = renderer;
    this->access = access;
    SDL_Texture* texture_raw = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, access, w, h);
    if (texture_raw == nullptr)
        throw std::runtime_error(std::string("Error: SDL_CreateTexture(): ") + SDL_GetError());
    texture = std::shared_ptr<SDL_Texture>(texture_raw, SDL_DestroyTexture);
}

Texture::Texture(SDL_Renderer* renderer, SDL_Surface* surface) {
    this->renderer = renderer;
    this->access = SDL_TEXTUREACCESS_STATIC;
    SDL_Texture* texture_raw = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture_raw == nullptr)
        throw std::runtime_error(std::string("Error: SDL_CreateTextureFromSurface(): ") + SDL_GetError());
    texture = std::shared_ptr<SDL_Texture>(texture_raw, SDL_DestroyTexture);
}

void Texture::fill(ImVec4 color) {
    switch (access) {
        case SDL_TEXTUREACCESS_STREAMING:
        {
            unsigned char* texture_bytes;
            int pitch;
            ImVec4 color_scaled = scaleVec(color, 255);
            SDL_LockTexture(texture.get(), NULL, (void**)&texture_bytes, &pitch);
            Uint32 rgba_src = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, color_scaled.x, color_scaled.y, color_scaled.z, color_scaled.w);
            for (int y = 0; y < texture->w; y++) {
                for (int x = 0; x < texture->h; x++) {
                    Uint32* rgba_dest = (Uint32*)&texture_bytes[y * pitch + x * sizeof(Uint32)];
                    *rgba_dest = rgba_src;
                }
            }
            SDL_UnlockTexture(texture.get());
            break;
        }
        case SDL_TEXTUREACCESS_TARGET:
        {
            setRenderTarget();
            SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, color.w);
            SDL_RenderClear(renderer);
            break;
        }
    }
}

void Texture::drawLine(ImVec2 start, ImVec2 end, ImVec4 color) {
    if (access == SDL_TEXTUREACCESS_TARGET) {
        // Set draw color
        SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, color.w);
    
        // Set target to draw on canvas texture rather than window
        setRenderTarget();
        
        // Draw line
        SDL_RenderLine(renderer, start.x, start.y, end.x, end.y);
    }
}

void Texture::loadFromArray(unsigned char* data) {
    switch (access) {
        case SDL_TEXTUREACCESS_STREAMING:
        {
            unsigned char* texture_bytes;
            int pitch;
            SDL_LockTexture(texture.get(), NULL, (void**)&texture_bytes, &pitch);
            for (int y = 0; y < texture->h; y++) {
                for (int x = 0; x < texture->w; x++) {
                    unsigned char* rgb_src = &data[(y * texture->w + x) * 3];
                    Uint32 rgba_src = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, rgb_src[0], rgb_src[1], rgb_src[2], 255);
                    Uint32* rgba_dest = (Uint32*)&texture_bytes[y * pitch + x * sizeof(Uint32)];
                    *rgba_dest = rgba_src;
                }
            }
            SDL_UnlockTexture(texture.get());
            break;
        }
        case SDL_TEXTUREACCESS_TARGET:
        {
            Texture streaming_texture(renderer, SDL_TEXTUREACCESS_STREAMING, texture->w, texture->h);
            streaming_texture.loadFromArray(data);
            streaming_texture.renderTo(*this, nullptr, nullptr);
            break;
        }
    }
}

void Texture::renderTo(Texture dest, const SDL_FRect* src_rect, const SDL_FRect* dest_rect) {
    if (dest.access == SDL_TEXTUREACCESS_TARGET) {
        SDL_SetRenderTarget(renderer, dest.get());
        SDL_RenderTexture(renderer, texture.get(), src_rect, dest_rect);
    }
}

void Texture::stampTextureAlongLine(Texture src, ImVec2 start, ImVec2 end) {
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    
    double dt = 1 / std::max(std::abs(dx), std::abs(dy));
    
    for (double t = 0; t < 1.0f; t += dt) {
        SDL_FRect dest_rect{
            (float)(int)(start.x + dx * t - src.width() / 2.0f),
            (float)(int)(start.y + dy * t - src.height() / 2.0f),
            (float)(int)src.width(),
            (float)(int)src.height()
        };
        src.renderTo(*this, NULL, &dest_rect);
    }
}

void Texture::setRenderTarget() {
    SDL_SetRenderTarget(renderer, texture.get());
}
