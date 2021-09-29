// General raytracing logic

#pragma once

#include <optional>

#include "vec.hpp"

#include "canvas.hpp"
#include "primitives.hpp"
#include "scene.hpp"

constexpr float kEpsilon = 0.001F;
constexpr unsigned int kMaxRecursionDepth = 2U;

// Scale the provided color struct (excluding alpha) by the provided intensity, with saturation
sf::Color scale_color(const sf::Color& color, float intensity) {
    return sf::Color(std::clamp(static_cast<int>(color.r * intensity), 0, 255),
                     std::clamp(static_cast<int>(color.g * intensity), 0, 255),
                     std::clamp(static_cast<int>(color.b * intensity), 0, 255));
}

// Reflect the provided vector across the provided unit-length normal vector
vec::Vec3f reflect_across_normal(const vec::Vec3f& ray_vector, const vec::Vec3f& normal) {
    return 2.0F * project_onto_unit(ray_vector, normal) - ray_vector;
}

// Get the ray-shape intersect and a reference to the associated shape. By default, returns the
// closest intersect, but can also be specified to return the first intersect found.
struct RayShapeIntersectData {
    float t;
    const Shape& shape;
};
std::optional<RayShapeIntersectData> calc_ray_shape_intersect(const vec::Vec3f& point,
                                                              const vec::Vec3f& vector,
                                                              float t_min,
                                                              float t_max,
                                                              bool find_closest= true) {
    // Closest sphere intersect data
    float closest_t = std::numeric_limits<float>::infinity();
    std::optional<std::reference_wrapper<const Shape>> closest_shape;

    // TODO: iterate across all shapes as added
    for (const Shape& shape_object : kSceneSpheres) {
        // Get ray-shape_object intersect points, if any
        const Sphere::RayIntersect intersects = shape_object.calc_ray_intersect(point, vector);
        for (float t : intersects) {
            // Check first intersect against provided range
            if ((t > t_min) && (t < t_max)) {
                if (find_closest) {
                    // Check if intersect is closer than any previous, then continue
                    if (t < closest_t) {
                        closest_t = t;
                        closest_shape = shape_object;
                    }
                } else {
                    // Any intersect is sufficient; break early
                    closest_t = t;
                    closest_shape = shape_object;
                    break;
                }
            }
        }
    }

    if (closest_shape) {
        return RayShapeIntersectData{closest_t, (*closest_shape).get()};
    } else {
        return std::nullopt;
    }
}

// Compute basic lighting intensity at the specified point and surface normal
float compute_lighting(vec::Vec3f point, vec::Vec3f normal, vec::Vec3f ray, float specularity) {
    float intensity = 0.0F;

    for (const Light& light : kSceneLights) {

        if (light.type == Light::Type::kAmbient) {
            // Simply add ambient light intensity
            intensity += light.intensity;
        } else {
            // Get light direction and max occlusion t
            vec::Vec3f direction;
            float max_t_occlusion;

            if (light.type == Light::Type::kDirectional) {
                direction = light.direction;
                max_t_occlusion = std::numeric_limits<float>::infinity();
            } else {
                direction = light.position - point;
                max_t_occlusion = 1.0F;
            }

            // Check for clear line of sight to the light source
            // TODO: Decrease light intensity with range? Inverse square law?
            if (!calc_ray_shape_intersect(point, direction, kEpsilon, max_t_occlusion, false)) {
                // Diffuse lighting
                const float normal_dot_direction = dot(normal, direction);
                if (normal_dot_direction > 0.0F) {
                    // Calculate and add light intensity according to angle of incidence
                    intensity += light.intensity * normal_dot_direction /
                                 (normal.euclidean() * direction.euclidean());
                }

                // Specular lighting
                // Non-physical model: intensity is cos(alpha)^specularity, where alpha is the angle
                // between the reflection vector and the negative of the ray vector we're tracing
                if (specularity > 0.0F) {
                    const vec::Vec3f reflection = reflect_across_normal(direction, normal);
                    const float reflection_dot_ray = dot(reflection, -ray);
                    if (reflection_dot_ray > 0.0F) {
                        const float reflection_ray_angle = reflection_dot_ray /
                                                           (reflection.euclidean() * ray.euclidean());
                        intensity += light.intensity * std::pow(reflection_ray_angle, specularity);
                    }
                }
            }
        }
    }

    return intensity;
}

// Trace ray using provided point, vector, and independent variable range.
// Calculate the color and lighting for the closest intersect point, if any.
sf::Color trace_ray(const vec::Vec3f& ray_point,
                    const vec::Vec3f& ray_vector,
                    float t_min,
                    float t_max,
                    unsigned int recursion_depth = kMaxRecursionDepth) {

    // Get the closest sphere intersect, if any
    const auto closest_intersect =
            calc_ray_shape_intersect(ray_point, ray_vector, t_min, t_max);

    if (closest_intersect) {
        // Apply lighting intensity to sphere color at intersect
        const vec::Vec3f intersect_point = ray_point + closest_intersect->t * ray_vector;
        const vec::Vec3f normal = closest_intersect->shape.calc_normal(intersect_point);
        const float intensity = compute_lighting(intersect_point,
                                                 normal,
                                                 ray_vector,
                                                 closest_intersect->shape.material.specularity);
        const sf::Color local_color = scale_color(closest_intersect->shape.material.color,
                                                  intensity);

        // Handle reflectivity via recursive raytracing
        const float reflectiveness = closest_intersect->shape.material.reflectiveness;
        if ((recursion_depth > 0) && (reflectiveness > 0.0F)) {
            vec::Vec3f reflected_ray = reflect_across_normal(-ray_vector, normal);
            const sf::Color reflected_color = trace_ray(intersect_point,
                                                        reflected_ray,
                                                        kEpsilon * closest_intersect->t,
                                                        std::numeric_limits<float>::infinity(),
                                                        recursion_depth - 1);
            return scale_color(local_color, (1.0F - reflectiveness)) +
                   scale_color(reflected_color, reflectiveness);
        } else {
            return local_color;
        }
    } else {
        // Background color for no intersect
        return sf::Color::White;
    }
}
