// General raytracing logic

#pragma once

#include <optional>

#include "vec.hpp"

#include "canvas.hpp"
#include "primitives.hpp"
#include "scene.hpp"

constexpr unsigned int kMaxRecursionDepth = 3U;

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
template <bool find_closest=true>
std::optional<RayShapeIntersectData> calc_ray_shape_intersect(const Ray& ray,
                                                              float t_min,
                                                              float t_max) {
    // Closest sphere intersect data
    float closest_t = std::numeric_limits<float>::infinity();
    std::optional<std::reference_wrapper<const Shape>> closest_shape;

    auto find_closest_shape = [&](const auto& objects) {
        for (const Shape& shape_object : objects) {
            // Get ray-shape intersection points, if any
            const Shape::RayIntersect intersect_points = shape_object.calc_ray_intersect(ray);
            for (float t : intersect_points) {
                // Check intersection point against provided range
                if ((t > t_min) && (t < t_max)) {
                    if constexpr (find_closest) {
                        // Check if intersection is closer than any previous, then continue
                        if (t < closest_t) {
                            closest_t = t;
                            closest_shape = shape_object;
                        }
                    } else {
                        // Any intersection is sufficient; break early
                        closest_t = t;
                        closest_shape = shape_object;
                        break;
                    }
                }
            }
        }
    };

    find_closest_shape(kSceneSpheres);
    find_closest_shape(kScenePlanes);

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
            if (!calc_ray_shape_intersect<false>({point, direction}, kEpsilon, max_t_occlusion)) {
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

vec::Vec3f calc_refraction_vector(const vec::Vec3f& incoming,
                                  const vec::Vec3f& normal,
                                  float refractivity) {
    if (refractivity <= 0.0F) {
        return incoming;
    }

    // Determine if light is entering or exiting object
    const bool entering_shape = dot(normal, incoming) < 0.0F;

    // Normalize inputs and set signs according to whether we're entering or exiting
    const vec::Vec3f arrival_unit = -incoming.normalize();
    const vec::Vec3f normal_unit = entering_shape ? normal.normalize() : -normal.normalize();
    const float refractive_ratio = entering_shape ?
                                   1.0F / (1.0F + refractivity) :
                                   1.0F + refractivity;

    // Compute refraction transmission vector
    const float normal_dot_arrival = dot(normal_unit, arrival_unit);
    const float temp = 1.0F - (normal_dot_arrival * normal_dot_arrival);
    const float in_radical = 1.0F - (refractive_ratio * refractive_ratio * temp);

    if (in_radical < 0.0F) {
        // Total internal reflection
        return reflect_across_normal(arrival_unit, normal_unit);
    }

    const float radical = std::sqrt(in_radical);

    const vec::Vec3f first = (refractive_ratio * normal_dot_arrival - radical) * normal_unit;
    const vec::Vec3f second = -refractive_ratio * arrival_unit;

    return first + second;
}

// Trace ray using provided point, vector, and independent variable range.
// Calculate the color and lighting for the closest intersect point, if any.
sf::Color trace_ray(const Ray& ray,
                    float t_min,
                    float t_max,
                    unsigned int recursion_depth = kMaxRecursionDepth) {

    // Get the closest sphere intersect, if any
    const auto closest_intersect =
            calc_ray_shape_intersect(ray, t_min, t_max);

    if (closest_intersect) {
        // Get useful intersect data
        const vec::Vec3f intersect_point = ray.calc_point(closest_intersect->t);
        const vec::Vec3f normal = closest_intersect->shape.calc_normal(intersect_point);
        const Material& material = closest_intersect->shape.material;

        // Handle reflectivity via recursive raytracing with a reflected vector
        const float reflectivity = material.reflectivity;
        sf::Color reflected_color_blend{};
        if ((recursion_depth > 0) && (reflectivity > 0.0F)) {
            const Ray reflected_ray = {intersect_point, reflect_across_normal(-ray.vector, normal)};
            const sf::Color reflected_color = trace_ray(reflected_ray,
                                                        kEpsilon * closest_intersect->t,
                                                        std::numeric_limits<float>::infinity(),
                                                        recursion_depth-1);
            reflected_color_blend = scale_color(reflected_color, reflectivity);
        }

        // Handle transparency via recursive raytracing with a continuing vector
        const float transparency = material.transparency;
        sf::Color transparent_color_blend{};
        if ((recursion_depth > 0) && (transparency > 0.0F)) {
            // TODO: determine ray direction based on refractivity

            const Ray continuing_ray = {intersect_point,
                                        calc_refraction_vector(ray.vector,
                                                               normal,
                                                               material.refractivity)};
            const sf::Color transparent_color = trace_ray(continuing_ray,
                                                          kEpsilon,
                                                          std::numeric_limits<float>::infinity(),
                                                          recursion_depth-1);
            transparent_color_blend = scale_color(transparent_color, transparency);
        }

        // Blend local color with reflected and transparent colors
        const sf::Color local_color_blend = scale_color(material.color,
                                                        1.0F - reflectivity - transparency);
        const sf::Color blend = local_color_blend + reflected_color_blend + transparent_color_blend;

        // Compute and apply lighting intensity to blended color
        const float light_intensity = compute_lighting(intersect_point,
                                                       normal,
                                                       ray.vector,
                                                       material.specularity);
        return scale_color(blend, light_intensity);
    } else {
        // Background color for no intersect
        return sf::Color::White;
    }
}
