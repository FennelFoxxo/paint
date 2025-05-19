#include "texture.hpp"

#include <string>
#include <stdexcept>

Texture::Texture(SDL_Renderer* renderer, SDL_TextureAccess access, int w, int h) {
    this->renderer = renderer;
    this->access = access;
    SDL_Texture* texture_raw = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, access, w, h);
    if (texture_raw == nullptr)
        throw std::runtime_error(std::string("Error: SDL_CreateTexture(): ") + SDL_GetError());
    texture = std::shared_ptr<SDL_Texture>(texture_raw, SDL_DestroyTexture);
}

void Texture::fill(unsigned char r, unsigned char g, unsigned char b) {
    switch (access) {
        case SDL_TEXTUREACCESS_STREAMING:
        {
            unsigned char* texture_bytes;
            int pitch;
            SDL_LockTexture(texture.get(), NULL, (void**)&texture_bytes, &pitch);
            Uint32 rgba_src = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), nullptr, r, g, b, 255);
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
            Texture src_pixel(renderer, SDL_TEXTUREACCESS_STREAMING, 1, 1);
            src_pixel.fill(r, g, b);
            src_pixel.renderTo(*this, nullptr, nullptr);
            break;
        }
    }
}
#include <iostream>
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

void Texture::renderTo(Texture& dest, const SDL_FRect* src_rect, const SDL_FRect* dest_rect) {
    if (dest.access == SDL_TEXTUREACCESS_TARGET) {
        SDL_SetRenderTarget(renderer, dest.get());
        SDL_RenderTexture(renderer, texture.get(), src_rect, dest_rect);
        SDL_SetRenderTarget(renderer, nullptr);
    }
}