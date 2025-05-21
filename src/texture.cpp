#include "texture.hpp"
#include "utils.hpp"

#include <string>
#include <stdexcept>
#include <cmath>

// Texture constructor for new blank texture given width and height
Texture::Texture(SDL_Renderer* renderer, SDL_TextureAccess access, int w, int h) {
    this->renderer = renderer;
    this->access = access;
    
    // Create texture using 4 bytes per pixel, RGBA mode
    SDL_Texture* texture_raw = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, access, w, h);
    
    // Throw error if texture could not be created
    if (texture_raw == nullptr)
        throw std::runtime_error(std::string("Error: SDL_CreateTexture(): ") + SDL_GetError());
        
    // Create shared pointer with custom allocator, will automatically destroy texture
    texture = std::shared_ptr<SDL_Texture>(texture_raw, SDL_DestroyTexture);
}

// Texture constructor for new texture created from an existing surface
Texture::Texture(SDL_Renderer* renderer, SDL_Surface* surface) {
    this->renderer = renderer;
    this->access = SDL_TEXTUREACCESS_STATIC; // Textures created from surface can only have static access mode
    
    // Create texture from surface
    SDL_Texture* texture_raw = SDL_CreateTextureFromSurface(renderer, surface);
    
    // Throw error if texture could not be created
    if (texture_raw == nullptr)
        throw std::runtime_error(std::string("Error: SDL_CreateTextureFromSurface(): ") + SDL_GetError());
    
    // Create shared pointer with custom allocator, will automatically destroy textur
    texture = std::shared_ptr<SDL_Texture>(texture_raw, SDL_DestroyTexture);
}

// Fill texture with solid color
void Texture::fill(ImVec4 color) {
    switch (access) {
        case SDL_TEXTUREACCESS_STREAMING:
        // With streaming access mode, we can edit the pixels directly but cannot render to the texture
        {
            unsigned char* texture_bytes;
            int pitch;
            // Convert the given color to the format that SDL expects
            Uint32 rgba_src = vecToUint32(SDL_PIXELFORMAT_RGBA8888, scaleVec(color, 255));
            
            // Lock the texture for editing
            SDL_LockTexture(texture.get(), NULL, (void**)&texture_bytes, &pitch);
            
            // Iterate over each pixel in the texture
            for (int y = 0; y < texture->w; y++) {
                for (int x = 0; x < texture->h; x++) {
                    // Get pixel address
                    Uint32* rgba_dest = (Uint32*)&texture_bytes[y * pitch + x * sizeof(Uint32)];
                    
                    // Set color
                    *rgba_dest = rgba_src;
                }
            }
            
            // Unlock the texture
            SDL_UnlockTexture(texture.get());
            break;
        }
        case SDL_TEXTUREACCESS_TARGET:
        // With target access mode it's easy since we can render to the texture
        {
            // Set this texture as render target
            setRenderTarget();
            
            // Set draw color
            SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, color.w);
            
            //  Clear texture, fills texture with draw color
            SDL_RenderClear(renderer);
            break;
        }
    }
}

// Draw a one-pixel wide line from the start to the end position
void Texture::drawLine(ImVec2 start, ImVec2 end, ImVec4 color) {
    // Only support doing this with textures that support being set as a render target
    if (access == SDL_TEXTUREACCESS_TARGET) {
        // Set draw color
        SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, color.w);
    
        // Set target to draw on canvas texture rather than window
        setRenderTarget();
        
        // Draw line
        SDL_RenderLine(renderer, start.x, start.y, end.x, end.y);
    }
}

// Create a new texture from an array
void Texture::loadFromArray(unsigned char* data) {
    switch (access) {
        case SDL_TEXTUREACCESS_STREAMING:
        // In streaming mode, we set each pixel in the texture to the color of the same pixel in the data array
        {
            unsigned char* texture_bytes;
            int pitch;
            
            // Lock texture for editing
            SDL_LockTexture(texture.get(), NULL, (void**)&texture_bytes, &pitch);
            
            // Iterate over each pixel in the texture
            for (int y = 0; y < texture->h; y++) {
                for (int x = 0; x < texture->w; x++) {
                    // Address of pixel to source color from
                    // data should have no padding, so pitch = width * bytes_per_pixel
                    unsigned char* rgb_src = (unsigned char*)getPixel(data, texture->w * sizeof(Uint32), x, y);
                    
                    // Convert the color to the format that SDL expects
                    Uint32 rgba_src = vecToUint32(SDL_PIXELFORMAT_RGBA8888,
                        {(float)rgb_src[0], (float)rgb_src[1], (float)rgb_src[2], (float)rgb_src[3]}
                    );
                    
                    // Set pixel in destination
                    editPixel(texture_bytes, pitch, x, y, rgba_src);
                }
            }
            
            // Unlock texture
            SDL_UnlockTexture(texture.get());
            break;
        }
        case SDL_TEXTUREACCESS_TARGET:
        // In target mode it's easy - just create a new texture with streaming access mode, load the array data into that,
        // then render it to this texture
        {
            Texture streaming_texture(renderer, SDL_TEXTUREACCESS_STREAMING, texture->w, texture->h);
            streaming_texture.loadFromArray(data);
            streaming_texture.renderTo(*this, nullptr, nullptr);
            break;
        }
    }
}

// Render this texture to another one
void Texture::renderTo(Texture dest, const SDL_FRect* src_rect, const SDL_FRect* dest_rect) {
    // Only valid if the destination texture supports being set as a render target
    if (dest.access == SDL_TEXTUREACCESS_TARGET) {
        SDL_SetRenderTarget(renderer, dest.get());
        SDL_RenderTexture(renderer, texture.get(), src_rect, dest_rect);
    }
}

// Renders a provided texture to every point of this texture that is along a line
void Texture::stampTextureAlongLine(Texture src, ImVec2 start, ImVec2 end) {
    // Only valid if this texture supports being set as the render target
    if (access != SDL_TEXTUREACCESS_TARGET) return;
    
    // Width and height of line
    double dx = end.x - start.x;
    double dy = end.y - start.y;
    
    // Amount that the interpolation variable should step forward each iteration
    // If |width| > |height|, then step size should be 1 / width, otherwise 1/height
    double dt = 1 / std::max(std::abs(dx), std::abs(dy));
    
    // Interpolate from 0 to 1
    for (double t = 0; t < 1.0f; t += dt) {
        // Center texture around current point on line
        SDL_FRect dest_rect{
            (float)(int)(start.x + dx * t - src.width() / 2.0f),
            (float)(int)(start.y + dy * t - src.height() / 2.0f),
            (float)(int)src.width(),
            (float)(int)src.height()
        };
        // Render source texture to this one
        src.renderTo(*this, NULL, &dest_rect);
    }
}

// Set this texture as the render target
void Texture::setRenderTarget() {
    SDL_SetRenderTarget(renderer, texture.get());
}
