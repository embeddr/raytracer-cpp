// Raytracer scene data.
//
// For now, this is hard-coded as a set of containers for each primitive type. Eventually, move
// toward loading this data at runtime from a file, and provide a better mechanism for organizing
// and accessing this data throughout the raytracer. A singleton might be appropriate.

#pragma once

#include <vector>

#include "primitives.hpp"

// Scene spheres
const std::vector<Sphere> kSceneSpheres {{
   {
       .center = {0.0F, -1.0F, 3.0F},
       .radius = 1.0F,
       .color = sf::Color::Red,
       .specularity = 500.0F,
       .reflectiveness = 0.2F,
   },
   {
       .center = {2.0F, 0.0F, 4.0F},
       .radius = 1.0F,
       .color = sf::Color::Blue,
       .specularity = 500.0F,
       .reflectiveness = 0.3F,
   },
   {
       .center = {-2.0F, 0.0F, 4.0F},
       .radius = 1.0F,
       .color = sf::Color::Green,
       .specularity = 10.0F,
       .reflectiveness = 0.3F,
   },
   {
       .center = {0.0F, -5001.0F, 0.0F},
       .radius = 5000.0F,
       .color = sf::Color::Yellow,
       .specularity = 1000.0F,
       .reflectiveness = 0.2F,
   },
}};

// Scene lights
const std::vector<Light> kSceneLights {{
    Light::make_ambient(0.2F),
    Light::make_point(0.6F, {2.1F, 1.0F, 0.0F}),
    Light::make_directional(0.2F, {1.0F, 4.0F, 4.0F})
}};

