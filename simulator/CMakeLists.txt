cmake_minimum_required(VERSION 3.14)
project(Klick32_Simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Simulator Definition
add_compile_definitions(SIMULATOR)

# Auto-download and build SDL2 so you don't have to manually install it
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "Build a static SDL2 library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build a static SDL2 library" FORCE)

FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.5
)
FetchContent_MakeAvailable(SDL2)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include     # Mocks (Arduino.h, Print.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../include  # Project includes
)

# PlatformIO automatically includes all subdirectories of lib/
file(GLOB LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*")
foreach(dir ${LIB_DIRS})
    if(IS_DIRECTORY ${dir})
        include_directories(${dir})
    endif()
endforeach()

# Fetch U8g2 source
set(U8G2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.pio/libdeps/esp32-s3-devkitc-1/U8g2/src")
include_directories(${U8G2_DIR} ${U8G2_DIR}/clib)

file(GLOB_RECURSE U8G2_SOURCES 
    "${U8G2_DIR}/clib/*.c" 
    "${U8G2_DIR}/*.cpp"
)

# Fetch all source files except those deeply tied to ESP32
# (We compile Core and src, but mock out ESP32 specific implementations)
file(GLOB_RECURSE GAME_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/*.cpp"
)
list(REMOVE_ITEM GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/main.cpp")

# Simulator specific sources
set(SIM_SOURCES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SIM_SOURCES} ${GAME_SOURCES} ${U8G2_SOURCES})

# Link against the fetched SDL2 static library
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2-static)
