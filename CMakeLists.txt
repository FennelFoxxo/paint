cmake_minimum_required(VERSION 3.16)
project(crosshair)

add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_executable(output src/main.c)