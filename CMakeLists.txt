cmake_minimum_required(VERSION 3.17)
project(raytracer)

set(CMAKE_CXX_STANDARD 20)

# SFML
find_package(SFML 2.5 COMPONENTS graphics REQUIRED)

# Raytracer
add_executable(raytracer main.cpp)
target_link_libraries(raytracer sfml-graphics)
