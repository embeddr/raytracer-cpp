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
    Camera::make(vec::Transform3f::rotate_y(M_PI/8.0), {2.0F, 0.0F, 0.0F}),
    Camera::make(vec::Transform3f::rotate_y(-M_PI/8.0), {-2.0F, 0.0F, 0.0F}),
}};

// Scene materials
const std::unordered_map<std::string, Material> kSceneMaterials {
    {
        "red_translucent",
        {
            .color = sf::Color::Red,
            .specularity = 5.0F,
            .reflectivity = 0.0F,
            .transparency = 0.9F,
            .refractivity = 0.02F,
        }
    },
    {
        "blue",
        {
            .color = sf::Color::Blue,
            .specularity = 250.0F,
            .reflectivity = 0.3F,
        }
    },
    {
        "silver",
        {
            .color = {210, 210, 210},
            .specularity = 500.0F,
            .reflectivity = 0.6F,
        }
    },
    {
        "green_dull",
        {
            .color = {150, 250, 50},
            .specularity = 10.0F,
            .reflectivity = 0.05F,
        }
    },
};

// Scene planes
const std::vector<Plane> kScenePlanes {{
    Plane({0.0F, -1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, kSceneMaterials.at("green_dull")),
}};

// Scene spheres
const std::vector<Sphere> kSceneSpheres {{
    Sphere({0.0F, 0.0F, 3.0F}, 1.0F, kSceneMaterials.at("red_translucent")),
    Sphere({2.0F, 0.0F, 4.0F}, 1.0F, kSceneMaterials.at("silver")),
    Sphere({-2.0F, 0.0F, 4.0F}, 1.0F, kSceneMaterials.at("blue")),
}};

// Scene lights
const std::vector<Light> kSceneLights {{
    Light::make_ambient(0.4F),
    Light::make_point(0.6F, {2.1F, 1.0F, 0.0F}),
    Light::make_directional(0.2F, {1.0F, 4.0F, 4.0F})
}};

