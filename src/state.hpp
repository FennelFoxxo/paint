#pragma once

#include "gui_resource.hpp"
#include "texture.hpp"

#include <imgui.h>

#include <SDL3/SDL.h>

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
        // Width and height of new canvas to be created
        int width;
        int height;
    } new_info;
    
    struct OpenInfo {
        std::string fname;
    } open_info;
    
    struct SaveAsInfo {
    } save_as_info;
};

// Actions performed by the "Image" menu in the top menu bar
struct ImageActionInfo {
    enum Status {
        None,
        DoResize
    };
    Status status = None;
    
    struct ResizeInfo {
        // Width and height that the canvas should be resized to
        int width;
        int height;
    } resize_info;
};

// Keeps track of information about dragging with the right mouse button
enum class CanvasDraggingState {
    not_dragging,           // Not dragging, or the right mouse button is not being clicked
    dragging_canvas,        // The user started holding the right mouse button while over the canvas and the canvas should be dragged
    dragging_off_canvas,    // The user started holding the right mouse button while not over the canvas (e.g. some GUI element) and should be ignored
};

// Faciliate communication between GUI and backend
struct State {
    // Pass around GUI resources - the SDL renderer is stored in here and is needed for many texture-modifying functions
    GuiResource* gui_resource;
    
    bool should_quit = false;           // Set to true if the program should terminate
    int window_width, window_height;    // Size of the main window on the screen
    bool mouse_left_down;               // True if left mouse button is being held down
    float scroll;                       // Positive of mouse wheel is being scrolled up, negative if down
    ImVec2 mouse_pos;                   // XY position of mouse on the screen
    ImVec2 mouse_pos_old;               // This is just mouse_pos from the previous frame
    bool mouse_over_canvas;             // Is the mouse hovering over the canvas and not any GUI elements?
    
    // Information about whether the canvas is being dragged around or not
    bool mouse_right_dragging; // Raw mouse dragging info returned by ImGui - true if RMB is held and mouse is moving
    CanvasDraggingState canvas_dragging_state = CanvasDraggingState::not_dragging;
    ImVec2 drag_delta{0, 0}; // Amount mouse has dragged since RMB started being held
    ImVec2 drag_delta_old{0, 0}; // This is just drag_delta from previous frame, compared with drag_delta to see how much mouse moved
    
    float framerate; // FPS of window
    
    // Information about the viewport i.e. the area that the canvas is rendered to, outside of any GUI elements
    ImVec4 viewport; // Bounding box of viewport
    // Negative offset of canvas from center of viewport i.e. imagining the viewport is a camera pointed at the canvas, this is the coordinates of the camera
    ImVec2 viewport_offset{0, 0};
    float scale = 1; // How much the canvas should be scaled up or down
    
    // Size of the menu on the right that holds drawing tools
    int right_menu_width = 200;
    
    ImVec4 clear_color;
    
    // Selected paint color - values are floats between 0 and 1
    ImVec4 draw_color = {1, 0, 0, 1};
    
    // GUI window visibility flags
    bool show_resize_window = false;
    bool show_new_file_window = false;
    
    // Actions requested by the user, passed from the GUI
    FileActionInfo file_action_info;
    ImageActionInfo image_action_info;
    
    // Texture object of the area that can be drawn to
    Texture canvas;
};