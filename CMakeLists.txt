cmake_minimum_required(VERSION 3.10)
project(NesEmulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

include_directories(include ${SDL2_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(nes_emu ${SOURCES})
target_link_libraries(nes_emu ${SDL2_LIBRARIES})
