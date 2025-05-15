cmake_minimum_required(VERSION 3.16)
project(crosshair)

add_compile_options(-static -static-libstdc++ -static-libgcc)
set(SDL_STATIC ON CACHE BOOL "SDL static linking")

add_subdirectory(sdl EXCLUDE_FROM_ALL)


add_library(imgui STATIC)
target_sources(imgui PRIVATE
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_sdl3.cpp
	imgui/backends/imgui_impl_sdlrenderer3.cpp
)
target_link_libraries(imgui PRIVATE SDL3::SDL3-static)
target_include_directories(imgui PUBLIC imgui imgui/backends)

add_executable(crosshair src/main.cpp)
target_link_libraries(crosshair PRIVATE SDL3::SDL3-static imgui)

install(TARGETS crosshair
	RUNTIME DESTINATION .
)