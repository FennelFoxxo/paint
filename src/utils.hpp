#pragma once

#include <imgui.h>

static ImVec2 canvasToScreenPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point) {
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

static ImVec2 screenToCanvasPos(ImVec2 canvas_size, ImVec4 viewport, ImVec2 viewport_offset, float scale, ImVec2 point) {
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