cmake_minimum_required(VERSION 3.17)
project(raytracer)

set(CMAKE_CXX_STANDARD 20)

# External dependencies
find_package(SFML 2.5 COMPONENTS graphics REQUIRED)
find_package(Threads REQUIRED)

# Vec dependency (git submodule)
set(VEC_INCLUDE_DIR vec/include/)
add_library(vec INTERFACE)
target_include_directories(vec INTERFACE ${VEC_INCLUDE_DIR})

# Raytracer
add_executable(raytracer
        src/main.cpp
        src/canvas.hpp
        src/primitives.hpp
        src/raytrace.hpp
        src/scene.hpp)
target_link_libraries(raytracer sfml-graphics vec Threads::Threads)
