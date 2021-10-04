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
        return Camera{.transform = vec::Transform3f(orientation, position)};
    }
};

struct Ray {
    vec::Vec3f point;
    vec::Vec3f vector;

    vec::Vec3f calc_point(float t) const {
        return point + (t * vector);
    }
};

struct Material {
    sf::Color color;
    float specularity;    // [0.0F to disable, else positive]
    // Note: sum of reflectivity and transparency must be <= 1.0F
    float reflectivity;   // [0.0F - 1.0F]
    float transparency;   // [0.0F - 1.0F]
    float refractivity;   // [0.0F to disable, else positive]
};

struct Shape {
    // Generic shape data
    Material material;

    explicit Shape(const Material& material) : material(material) {}

    // Get vector of ray-shape intersect point(s), if any
    using RayIntersect = std::vector<float>;
    virtual RayIntersect calc_ray_intersect(const Ray& ray) const = 0;

    // Get normal vector given shape surface point
    virtual vec::Vec3f calc_normal(const vec::Vec3f& surface_point) const = 0;
};

struct Sphere : public Shape {
    // Sphere-specific shape data
    vec::Vec3f center;
    float radius;

    Sphere(const vec::Vec3f& center, float radius, const Material& material) :
        Shape(material),
        center(center),
        radius(radius) {}

    // Get ray-sphere intersect points, if any
    RayIntersect calc_ray_intersect(const Ray& ray) const override {
        const float r = radius;
        const vec::Vec3f c_p = ray.point - center;

        const float a = dot(ray.vector, ray.vector);
        const float b = dot(c_p, ray.vector) * 2.0F;
        const float c = dot(c_p, c_p) - (r*r);

        const float discriminant = (b*b) - (4*a*c);
        if (discriminant < 0.0F) {
            // No intersect point
            return {};
        }

        const float disc_root = std::sqrt(discriminant);

        return std::vector<float>{
            (-b + disc_root) / (2*a),
            (-b - disc_root) / (2*a)
        };
    }

    vec::Vec3f calc_normal(const vec::Vec3f& surface_point) const override {
        return (surface_point - center).normalize();
    }
};

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
