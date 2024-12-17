cmake_minimum_required(VERSION 3.16)
project(crosshair)

add_subdirectory(sdl_src EXCLUDE_FROM_ALL)

add_executable(crosshair src/main.c)
target_link_libraries(crosshair PRIVATE SDL3::SDL3-static)

install(TARGETS crosshair
	RUNTIME DESTINATION .
)