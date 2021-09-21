cmake_minimum_required(VERSION 3.17)
project(raytracer)

set(CMAKE_CXX_STANDARD 20)

# SFML
find_package(SFML 2.5 COMPONENTS graphics system REQUIRED)

# Vec
set(VEC_INCLUDE_DIR vec/include/)
add_library(vec INTERFACE)
target_include_directories(vec INTERFACE ${VEC_INCLUDE_DIR})

# Raytracer
add_executable(raytracer main.cpp)
target_link_libraries(raytracer sfml-graphics sfml-system vec)
