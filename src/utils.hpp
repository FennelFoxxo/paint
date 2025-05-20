#pragma once

#include <imgui.h>
#include <SDL3/SDL.h>
#include <nfd.h>

#include <vector>
#include <string>

// Convert a position from canvas space to screen space
ImVec2 canvasToScreenPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point);

// Convert a position from screen space to canvas space
ImVec2 screenToCanvasPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point);

// Multiply each element of a vector of 4 floats by another float
ImVec4 scaleVec(ImVec4 in, float scale);

// Draw the outline of a circle centered on the surface
void drawCircle(SDL_Surface* surface, int radius, ImVec4 color);

// Paint bucket tool - starting as pos, fill all matching colors with draw color
void floodFill(SDL_Surface* surface, ImVec2 pos, ImVec4 draw_color_vec);

// Opens a file explorer GUI that lets the user pick a file to open from/save to
// Set save to true if a save dialog should be opened which lets the user type in a filename,
// or false to force the user to select an existing file
std::string requestFileDialog(std::vector<nfdu8filteritem_t> filters, bool save);

// Open an image file at given path and create surface from image data
SDL_Surface* openImage(std::string path);

// Save surface image data at given path
void saveImage(std::string path, SDL_Surface* surface);