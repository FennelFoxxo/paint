#pragma once

#include "gui_resource.hpp"
#include "texture.hpp"
#include "utils.hpp"

#include <imgui.h>

#include <SDL3/SDL.h>

// Forward declaration
struct State;

// Some structs to facilitate communication between the GUI and backend

// Actions performed by the "File" menu in the top menu bar
struct FileActionInfo {
    enum Status {
        None,
        DoNew,
        DoOpen,
        DoSaveAs
    };
    Status status = None;
    
    struct NewInfo {
        // Size of new canvas to be created
        ImVec2 size;
    } new_info;
};

// Actions performed by the "Image" menu in the top menu bar
struct ImageActionInfo {
    enum Status {
        None,
        DoResize
    };
    Status status = None;
    
    struct ResizeInfo {
        // Size that the canvas should be resized to
        ImVec2 size;
    } resize_info;
};

struct MousePos {
    ImVec2 screen; // XY position of mouse on the screen
    ImVec2 canvas; // XY position of mouse on the canvas
    
    void updateCanvasPos(State* state);
};

// Keeps track of information about mouse dragging
struct MouseButtonInfo {
    // Is mouse button down?
    bool down = false;

    // Position of mouse when button started being held
    MousePos drag_start;
};

// Keep track of which tool the user has selected for drawing
enum class DrawingTool {
    Brush,
    Line,
    Fill
};

// Faciliate communication between GUI and backend
struct State {
    // CONSTANTS
    
    // Color used to fill in background where no elements are drawn
    const ImVec4 clear_color{0.45f, 0.55f, 0.60f, 1.00f};
    
    // Width and height of initial canvas
    const ImVec2 initial_canvas_size{500.0f, 500.0f};
    
    // Background color of icon texture when it is selected
    const ImVec4 selected_icon_color{1, 1, 1, 1};
    
    // Background color of icon texture when it is not selected
    const ImVec4 unselected_icon_color{0.3, 0.3, 0.3, 1};
    
    
    // END CONSTANTS
    
    
    // Pass around GUI resources - the SDL renderer is stored in here and is needed for many texture-modifying functions
    GuiResource* gui_resource;
    
    bool should_quit = false;           // Set to true if the program should terminate
    int window_width, window_height;    // Size of the main window on the screen
    float scroll;                       // Positive of mouse wheel is being scrolled up, negative if down
    bool gui_wants_mouse;               // Is the mouse hovering or dragging over any GUI elements?
    
    MousePos mouse_pos;     // Position of the mouse    
    MousePos mouse_pos_old; // Position of the mouse from the previous frame
    
    // Info about mouse buttons, along with the same info from the previous frame
    MouseButtonInfo lmb_info;
    MouseButtonInfo rmb_info;
    MouseButtonInfo lmb_info_old;
    MouseButtonInfo rmb_info_old;
    
    // Info about the current line being drawn when in line tool mode
    MousePos draw_line_start;
    MousePos draw_line_end;
    bool drawing_line = false;
    
    // Brush settings
    int brush_size = 15; // Brush width (diameter) in pixels
    bool brush_details_changed = false; // Has the user tweaked the brush size or color since the last frame?
    Texture brush_texture_preview; // Preview of brush size, circular outline with no fill
    Texture brush_texture; // Brush texture, circle with fill
    DrawingTool drawing_tool = DrawingTool::Brush; // Which tool has the user selected for drawing?
    
    float framerate; // FPS of window
    
    // Information about the viewport i.e. the area that the canvas is rendered to, outside of any GUI elements
    ImVec4 viewport; // Bounding box of viewport
    // Negative offset of canvas from center of viewport i.e. imagining the viewport is a camera pointed at the canvas, this is the coordinates of the camera
    ImVec2 viewport_offset{0, 0};
    // How much the canvas should be scaled up or down
    float scale = 1;
    
    // Width of the menu on the right that holds drawing tools
    int right_menu_width = 200;
    
    // User selected paint color - values are floats between 0 and 1
    ImVec4 draw_color{1, 0, 0, 1};
    
    // GUI window visibility flags
    bool show_resize_window = false;
    bool show_new_file_window = false;
    
    // Actions requested by the user, passed from the GUI
    FileActionInfo file_action_info;
    ImageActionInfo image_action_info;
    
    // Texture object of the area that can be drawn to
    Texture canvas;
    
    // Icon textures for drawing tool modes
    struct {
        Texture brush;
        Texture line;
        Texture fill;
    } icons;
};

// Synchronize canvas position from screen position
// It's such a short function that there's no point putting it in its own source file,
// so making it inline prevents multiple definition linker errors
inline void MousePos::updateCanvasPos(State* state) {
    canvas = screenToCanvasPos(state->canvas.size(), state->viewport, state->viewport_offset, state->scale, screen);
}