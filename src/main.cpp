#include "state.hpp"
#include "gui.hpp"
#include "backend.hpp"
#include "texture.hpp"
#include <iostream>


int main(int, char**)
{
    // Create object which represents lifetime of GUI libraries.
    // This initializes SDL and ImGui, along with creating a window and SDL renderer.
    // SDL and ImGui will be cleaned up when this object goes out of scope.
    GuiResource gui_resource("Paint", 1280, 960);   // Window title, width, height
    
    // Create state object which keeps track of communication between gui and backend
    State state{.gui_resource = &gui_resource};

    // Initializes the state and creates some required objects e.g. canvas and icon textures
    backendInit(&state);
    
    // Main loop
    while (!state.should_quit) {
        // Draw the GUI to the screen
        guiDraw(&state);
        
        // Renders the GUI to the window
        guiPresent(&state);
        
        // Process events that happened e.g. if user dragged mouse to draw
        backendProcess(&state);
    }

    return 0;
}