// Primitive scene object types, such as geometric shapes and lights.
//
// Anything that can be placed in the scene should be defined here. The actual scene contents are
// currently specified in scene.hpp, which uses these definitions.

#pragma once

#include <optional>

#include "vec.hpp"
#include "transform.hpp"

struct Camera {
    vec::Transform3f transform;

    static Camera make(vec::Mat3f orientation, vec::Vec3f position) {
        return Camera {.transform = vec::Transform3f(orientation, position)};
    }
};

// TODO: Define a base shape type to be used for finding intersects

// Sphere object
struct Sphere {
    vec::Vec3f center;
    float radius;
    sf::Color color;
    float specularity;    // [0.0F to disable, else positive]
    float reflectiveness; // [0.0F, 1.0F]

    // Get ray-sphere intersect points, if any
    using RayIntersect = std::optional<std::pair<float, float>>;
    RayIntersect calc_ray_intersect(const vec::Vec3f& ray_point,
                                    const vec::Vec3f& ray_vector) const {
        const float r = radius;
        const vec::Vec3f c_p = ray_point - center;

        const float a = dot(ray_vector, ray_vector);
        const float b = dot(c_p, ray_vector) * 2.0F;
        const float c = dot(c_p, c_p) - (r*r);

        const float discriminant = (b*b) - (4*a*c);
        if (discriminant < 0.0F) {
            // No intersect point
            return std::nullopt;
        }

        const float disc_root = std::sqrt(discriminant);

        return std::make_pair((-b + disc_root) / (2*a), (-b - disc_root) / (2*a));
    }

};

// Light object
struct Light {
    enum class Type {
        kAmbient,
        kPoint,
        kDirectional,
    };

    Type type;
    float intensity;
    vec::Vec3f position;  // point lights only
    vec::Vec3f direction; // directional lights only

    static Light make_ambient(float intensity) {
        return Light {
                .type = Type::kAmbient,
                .intensity = intensity,
                .position = {},
                .direction = {},
        };
    };

    static Light make_point(float intensity, vec::Vec3f position) {
        return Light {
                .type = Type::kPoint,
                .intensity = intensity,
                .position = position,
                .direction = {},
        };
    };

    static Light make_directional(float intensity, vec::Vec3f direction) {
        return Light {
                .type = Type::kDirectional,
                .intensity = intensity,
                .position = {},
                .direction = direction,
        };
    };
};
