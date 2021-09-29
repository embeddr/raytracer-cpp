// Raytracer scene data.
//
// For now, this is hard-coded as a set of containers for each primitive type. Eventually, move
// toward loading this data at runtime from a file, and provide a better mechanism for organizing
// and accessing this data throughout the raytracer. A singleton might be appropriate.

#pragma once

#include <vector>
#include <string>

#include "vec.hpp"
#include "transform.hpp"

#include "primitives.hpp"

constexpr vec::Vec3f kOrigin{0.0F, 0.0F, 0.0F};

// Scene cameras
const std::vector<Camera> kSceneCameras {{
    Camera::make(vec::Mat3f::identity(), kOrigin),
    Camera::make(vec::Transform3f::rotate_y(M_PI/4.0), {3.0F, 0.0F, 1.0F}),
}};

// Scene materials
const std::unordered_map<std::string, Material> kSceneMaterials {
    {
        "red_dull",
        {
            .color = sf::Color::Red,
            .specularity = 100.0F,
            .reflectiveness = 0.2F,
        }
    },
    {
        "blue",
        {
            .color = sf::Color::Blue,
            .specularity = 250.0F,
            .reflectiveness = 0.3F,
        }
    },
    {
        "silver",
        {
            .color = {200, 200, 200},
            .specularity = 500.0F,
            .reflectiveness = 0.5F,
        }
    },
    {
        "yellow_dull",
        {
            .color = sf::Color::Yellow,
            .specularity = 10.0F,
            .reflectiveness = 0.1F,
        }
    },
};


// Scene spheres
const std::vector<Sphere> kSceneSpheres {{
   {
       .center = {0.0F, -1.0F, 3.0F},
       .radius = 1.0F,
       .material = kSceneMaterials.at("red_dull"),
   },
   {
       .center = {2.0F, 0.0F, 4.0F},
       .radius = 1.0F,
       .material = kSceneMaterials.at("silver"),
   },
   {
       .center = {-2.0F, 0.0F, 4.0F},
       .radius = 1.0F,
       .material = kSceneMaterials.at("blue"),
   },
   {
       .center = {0.0F, -5001.0F, 0.0F},
       .radius = 5000.0F,
       .material = kSceneMaterials.at("yellow_dull"),
   },
}};

// Scene lights
const std::vector<Light> kSceneLights {{
    Light::make_ambient(0.2F),
    Light::make_point(0.6F, {2.1F, 1.0F, 0.0F}),
    Light::make_directional(0.2F, {1.0F, 4.0F, 4.0F})
}};

