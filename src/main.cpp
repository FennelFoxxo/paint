#include "state.hpp"
#include "gui.hpp"
#include "backend.hpp"

// Main code
int main(int, char**)
{
    GuiResource gui_resource("My window", 1280, 960);
    State state{.gui_resource = &gui_resource};

    backendInit(&state);

    // Main loop
    while (!state.should_quit) {
        guiUpdateStateMeta(&state);        
        
        guiDraw(&state);
        guiPresent(&state);
        
        backendProcess(&state);
    }

    return 0;
}