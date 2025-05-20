#include "utils.hpp"

#include <imgui.h>
#include <SDL3/SDL.h>
#include <nfd.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <vector>
#include <tuple>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <iostream>

// Convert a position from canvas space to screen space
ImVec2 canvasToScreenPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point) {
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

// Convert a position from screen space to canvas space
ImVec2 screenToCanvasPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point) {
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

// Multiply each element of a vector of 4 floats by another float
ImVec4 scaleVec(ImVec4 in, float scale) {
    return {in.x * scale, in.y * scale, in.z * scale, in.w * scale};
}

// Convert a vector of 4 floats to an RGBA value that SDL expects, based on provided format
Uint32 vecToUint32(SDL_PixelFormat format, ImVec4 in) {
    return SDL_MapRGBA(SDL_GetPixelFormatDetails(format), nullptr, in.x, in.y, in.z, in.w);
}

// Convert an SDL RGBA value to a vector of 4 floats, based on provided format
ImVec4 uint32ToVec(SDL_PixelFormat format, Uint32 in) {
    unsigned char r, g, b, a;
    SDL_GetRGBA(in, SDL_GetPixelFormatDetails(format), nullptr, &r, &g, &b, &a);
    return {(float)r, (float)g, (float)b, (float)a};
}

// Get address of a pixel at given position
Uint32* getPixel(void* array, int pitch, int x, int y) {
    return &((Uint32*)array)[y * pitch / sizeof(Uint32) + x];
}

// Modify the color of a pixel at given position
void editPixel(void* array, int pitch, int x, int y, Uint32 rgba) {
    *getPixel(array, pitch, x, y) = rgba;
}

// Draw the outline of a circle centered on the surface
void drawCircle(SDL_Surface* surface, int radius, ImVec4 color) {
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

// Paint bucket tool - starting as pos, fill all matching colors with draw color
void floodFill(SDL_Surface* surface, ImVec2 pos, ImVec4 draw_color_vec) {
    // Convert draw color to Uint32 that can be written into pixel array
    Uint32 draw_color = vecToUint32(surface->format, scaleVec(draw_color_vec, 255));
    
    // Color at provided position - only other pixels matching this color will be modified
    Uint32 starting_color = *getPixel(surface->pixels, surface->pitch, pos.x, pos.y); 
    
    // Stack of pixels that need to be filled in
    std::vector<std::tuple<int, int>> stack;
    
    // Start with initial position
    stack.push_back({pos.x, pos.y});
    
    while (!stack.empty()) {
        // Pop the last position off the stack
        auto [this_x, this_y] = stack[stack.size() - 1];
        stack.pop_back();
        
        // Skip if position is out of bounds
        if (this_x < 0 || this_x >= surface->w || this_y < 0 || this_y >= surface->h) continue;
        
        // Get address of pixel's color
        Uint32* this_pixel = getPixel(surface->pixels, surface->pitch, this_x, this_y);

        // Skip if pixel's color does not match starting color or if already filled in
        if (*this_pixel != starting_color || *this_pixel == draw_color) continue;
        
        // Update pixel color
        *this_pixel = draw_color;
        
        // Add neighboring pixels to stack to check
        stack.push_back({this_x+1, this_y});
        stack.push_back({this_x-1, this_y});
        stack.push_back({this_x, this_y+1});
        stack.push_back({this_x, this_y-1});
    }
}

// Opens a file explorer GUI that lets the user pick a file to open from/save to
// Set save to true if a save dialog should be opened which lets the user type in a filename,
// or false to force the user to select an existing file
std::string requestFileDialog(std::vector<nfdu8filteritem_t> filters, bool save) {
    // Variables for storing the output path picked by the user
    nfdu8char_t *out_path_raw;
    std::string out_path;
    
    // Use current directory as default path
    auto cwd = std::filesystem::current_path();
    
    nfdresult_t ret;
    if (save) {
        // Struct to store save dialog options
        nfdsavedialogu8args_t args = {0};
        
        // Populate args with filter specifying valid file types
        args.filterList = filters.data();
        args.filterCount = filters.size();
        
        args.defaultPath = cwd.c_str();
        
        // Open dialog and block until dialog closed
        ret = NFD_SaveDialogU8_With(&out_path_raw, &args);
    } else {
        // Struct to store open dialog options
        nfdopendialogu8args_t args = {0};
        
        // Populate args with filter specifying valid file types
        args.filterList = filters.data();
        args.filterCount = filters.size();
        
        args.defaultPath = cwd.c_str();
        
        // Open dialog and block until dialog closed
        ret = NFD_OpenDialogU8_With(&out_path_raw, &args);
    }
    
    
    
    
    if (ret == NFD_OKAY) { // Success, user picked a path
        // Convert raw path to C++ string
        out_path = std::string(out_path_raw);
        
        // The returned path is dynamically allocated and should be freed before exiting
        NFD_FreePathU8(out_path_raw);
        return out_path;
    }
    else if (ret == NFD_CANCEL) { // User pressed cancel, return empty string
        return "";
    }
    // Else if some other error occured
    throw std::runtime_error(std::string("Error: NFD_OpenDialogU8_With(): ") + NFD_GetError());
}

// Open an image file at given path and create surface from image data
SDL_Surface* openImage(std::string path) {
    // Load image data and request 4 channels
    int w, h;
    unsigned char *data = stbi_load(path.c_str(), &w, &h, nullptr, 4);
    if (data == nullptr)
        throw std::runtime_error("stbi_load()");
    
    SDL_Surface* image = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (image == nullptr) {
        stbi_image_free(data);
        throw std::runtime_error(std::string("Error: SDL_CreateSurface(): ") + SDL_GetError());
    }
        
    
    // Copy pixels from source array to destination surface and convert pixels to surface format
    for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
            // Get address of source pixel in data array
            // Since stbi_load never adds padding to the end of rows, we can assume pitch == width * bytes_per_pixel
            unsigned char* src_addr = (unsigned char*)getPixel(data, w * sizeof(Uint32), col, row);
            
            // Convert source pixel to color format that SDL expects
            Uint32 src_color = vecToUint32(image->format, {(float)src_addr[0], (float)src_addr[1], (float)src_addr[2], (float)src_addr[3]});
            
            // Modify pixel in surface
            editPixel(image->pixels, image->pitch, col, row, src_color);
        }
    }
    
    // Free image data
    stbi_image_free(data);
    
    std::cout << "Opened file " << path << std::endl;
    
    return image;
}

// Check if a string ends with another string
bool endsWith(const std::string& value, const std::string& ending) {
    // String too short to end with ending
    if (ending.size() > value.size()) return false;
    
    // Check if strings match by comparing in reverse
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// Save surface image data at given path
void saveImage(std::string path, SDL_Surface* surface) {
    // Create array to store image data
    auto data = std::make_unique<unsigned char[]>(surface->w * surface->h * sizeof(Uint32));
    
    // Copy pixels from source surface to destination array and convert pixels from surface format to flat RGBA
    for (int row = 0; row < surface->h; row++) {
        for (int col = 0; col < surface->w; col++) {
            // Get address of source pixel in surface
            Uint32* src_addr = getPixel(surface->pixels, surface->pitch, col, row);
            
            // Convert source pixel to from SDL color format to individual components
            ImVec4 src_color = uint32ToVec(surface->format, *src_addr);
            
            // Since stbi_write expects no padding at the end of rows, so we set pitch = width * bytes_per_pixel
            unsigned char* dest_addr = (unsigned char*)getPixel(data.get(), surface->w * sizeof(Uint32), col, row);
            
            // Set each component in destination array
            dest_addr[0] = src_color.x;
            dest_addr[1] = src_color.y;
            dest_addr[2] = src_color.z;
            dest_addr[3] = src_color.w;
        }
    }
    
    int ret;
    if (endsWith(path, ".png")) {
        // If path ends with .png
        ret = stbi_write_png(path.c_str(), surface->w, surface->h, 4, data.get(), surface->w * sizeof(Uint32));
    } else if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) {
        // If path ends with .jpg or .jpeg
        // Use maximum jpg quality of 100
        ret = stbi_write_jpg(path.c_str(), surface->w, surface->h, 4, data.get(), 100);
    } else {
        // If file type not recognized, save it as a png and add .png to the end
        path += ".png";
        ret = stbi_write_png(path.c_str(), surface->w, surface->h, 4, data.get(), surface->w * sizeof(Uint32));
    }
    
    if (ret == 0)
        throw std::runtime_error("Error: stbi_write()");
    std::cout << "Saved file as " << path << std::endl;
}