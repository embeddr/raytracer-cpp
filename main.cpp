// Standard lib
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>

// SFML
#include "SFML/Graphics.hpp"

// Vec
#include "vec.hpp"

// Constants
constexpr vec::Vec3f kOrigin{0.0F, 0.0F, 0.0F};

constexpr int kCanvasWidth = 800U;
constexpr int kCanvasHeight = 600U;

constexpr float kViewportWidth = 1.0F;
constexpr float kViewportHeight = 0.75F;
constexpr float kViewportDepth = 0.75F;

constexpr unsigned int kMaxRecursionDepth = 2U;

constexpr float kEpsilon = 0.001;

/******************************************************************************
 * SCENE TYPES AND DATA
 ******************************************************************************/

// TODO: load scene data from a file at runtime

// Basic sphere object
struct Sphere {
    vec::Vec3f center;
    float radius;
    sf::Color color;
    float specularity;
    float reflectiveness;
};

// Scene spheres
const std::array<Sphere, 4> kSceneSpheres {{
    {
        .center = vec::Vec3f{0.0F, -1.0F, 3.0F},
        .radius = 1.0F,
        .color = sf::Color::Red,
        .specularity = 500.0F,
        .reflectiveness = 0.2F,
    },
    {
        .center = vec::Vec3f{2.0F, 0.0F, 4.0F},
        .radius = 1.0F,
        .color = sf::Color::Blue,
        .specularity = 500.0F,
        .reflectiveness = 0.3F,
    },
    {
        .center = vec::Vec3f{-2.0F, 0.0F, 4.0F},
        .radius = 1.0F,
        .color = sf::Color::Green,
        .specularity = 10.0F,
        .reflectiveness = 0.3F,
    },
    {
        .center = vec::Vec3f{0.0F, -5001.0F, 0.0F},
        .radius = 5000.0F,
        .color = sf::Color::Yellow,
        .specularity = 1000.0F,
        .reflectiveness = 0.2F,
    },
}};

// Basic light object
struct Light {
    enum class Type {
        kAmbient,
        kPoint,
        kDirectional,
    };

    Type type;
    float intensity;
    vec::Vec3f position;
    vec::Vec3f direction;

    static Light makeAmbient(float intensity) {
        return Light {
            .type = Type::kAmbient,
            .intensity = intensity,
            .position = {},
            .direction = {},
        };
    };

    static Light makePoint(float intensity, vec::Vec3f position) {
        return Light {
            .type = Type::kPoint,
            .intensity = intensity,
            .position = position,
            .direction = {},
        };
    };

    static Light makeDirectional(float intensity, vec::Vec3f direction) {
        return Light {
            .type = Type::kDirectional,
            .intensity = intensity,
            .position = {},
            .direction = direction,
        };
    };
};

// Scene lights
const std::array<Light, 3> kSceneLights {{
    Light::makeAmbient(0.2F),
    Light::makePoint(0.6F, {2.1F, 1.0F, 0.0F}),
    Light::makeDirectional(0.2F, {1.0F, 4.0F, 4.0F})
}};

/******************************************************************************
 * CANVAS
 ******************************************************************************/

// Simple canvas abstraction to facilitate drawing individual pixels to the window
class Canvas : public sf::Drawable {
public:
    // Construct a canvas of specified dimensions and color (default black)
    Canvas(std::size_t width, std::size_t height, sf::Color color=sf::Color::Black) {
        image_.create(width, height, color);
        texture_.loadFromImage(image_);
        sprite_.setTexture(texture_);
    }

    // Place a pixel of the specified color at the specified coordinates
    // Origin is at center of canvas, positive x is right, positive y is up
    void put_pixel(int x, int y, sf::Color color) {
        auto size = image_.getSize();
        image_.setPixel((size.x / 2) + x, (size.y / 2) - y, color);
    }

    // Take a snapshot of all pixel updates so that they may be drawn
    // Must be called after placing pixels for changes to take effect
    void snapshot() {
        texture_.loadFromImage(image_);
    }

private:
    // Draw override to allow drawing the canvas directly
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        // You can draw other high-level objects
        target.draw(sprite_, states);
    }

    sf::Image image_;
    sf::Texture texture_;
    sf::Sprite sprite_;
};

/******************************************************************************
 * RAYTRACING
 ******************************************************************************/

// Scale the provided color struct (excluding alpha) by the provided intensity, with saturation
sf::Color scaleColor(const sf::Color& color, float intensity) {
    return sf::Color(std::clamp(static_cast<int>(color.r * intensity), 0, 255),
                     std::clamp(static_cast<int>(color.g * intensity), 0, 255),
                     std::clamp(static_cast<int>(color.b * intensity), 0, 255));
}

// Reflect the provided ray vector across the provided normal vector
vec::Vec3f reflectRay(const vec::Vec3f& ray_vector, const vec::Vec3f& normal) {
    return 2.0F * project_onto_unit(ray_vector, normal) - ray_vector;
}

// Get sphere intersect points
using RaySphereIntersect = std::optional<std::pair<float, float>>;
RaySphereIntersect calcRaySphereIntersect(const vec::Vec3f& ray_point,
                                          const vec::Vec3f& ray_vector,
                                          const Sphere& sphere) {
    const float r = sphere.radius;
    const vec::Vec3f c_p = ray_point - sphere.center;

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

// Get the closest ray-sphere intersect, and a reference to the associated sphere
struct ClosestRaySphereIntersect {
    float t;
    const Sphere& sphere;
};
std::optional<ClosestRaySphereIntersect> calcClosestRaySphereIntersect(const vec::Vec3f& point,
                                                                       const vec::Vec3f& vector,
                                                                       float t_min,
                                                                       float t_max) {
    // Closest sphere intersect data
    float closest_t = std::numeric_limits<float>::infinity();
    std::optional<std::reference_wrapper<const Sphere>> closest_sphere;

    for (const Sphere& sphere : kSceneSpheres) {
        // Get ray-sphere intersect points, if any
        const RaySphereIntersect t = calcRaySphereIntersect(point, vector, sphere);
        if (t) {
            // Check ranges
            if ((t->first > t_min) && (t->first < t_max) && t->first < closest_t) {
                closest_t = t->first;
                closest_sphere = sphere;
            }
            if ((t->second > t_min) && (t->second < t_max) && t->second < closest_t) {
                closest_t = t->second;
                closest_sphere = sphere;
            }
        }
    }

    if (closest_sphere) {
        return ClosestRaySphereIntersect{closest_t, (*closest_sphere).get()};
    } else {
        return std::nullopt;
    }
}

// Compute basic lighting intensity at the specified point and surface normal
float computeLighting(vec::Vec3f point, vec::Vec3f normal, vec::Vec3f ray, float specularity) {
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
            if (!calcClosestRaySphereIntersect(point, direction, kEpsilon, max_t_occlusion)) {
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
                    const vec::Vec3f reflection = reflectRay(direction, normal);
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
sf::Color traceRay(const vec::Vec3f& ray_point,
                   const vec::Vec3f& ray_vector,
                   float t_min,
                   float t_max,
                   unsigned int recursion_depth = kMaxRecursionDepth) {

    // Get the closest sphere intersect, if any
    const auto closest_intersect =
            calcClosestRaySphereIntersect(ray_point, ray_vector, t_min, t_max);

    if (closest_intersect) {
        // Apply lighting intensity to sphere color at intersect
        const vec::Vec3f intersect_point = ray_point + closest_intersect->t * ray_vector;
        const vec::Vec3f normal = (intersect_point - closest_intersect->sphere.center).normalize();
        const float intensity = computeLighting(intersect_point,
                                                normal,
                                                ray_vector,
                                                closest_intersect->sphere.specularity);
        const sf::Color local_color = scaleColor(closest_intersect->sphere.color, intensity);

        // Handle reflectivity via recursive raytracing
        const float reflectiveness = closest_intersect->sphere.reflectiveness;
        if ((recursion_depth > 0) && (reflectiveness > 0.0F)) {
            vec::Vec3f reflected_ray = reflectRay(-ray_vector, normal);
            const sf::Color reflected_color = traceRay(intersect_point,
                                                       reflected_ray,
                                                       0.1F, // Note: kEpsilon results in artifacts
                                                       std::numeric_limits<float>::infinity(),
                                                       recursion_depth - 1);
            return scaleColor(local_color, (1.0F - reflectiveness)) +
                    scaleColor(reflected_color, reflectiveness);
        } else {
            return local_color;
        }
    } else {
        // Background color for no intersect
        return sf::Color::White;
    }
}

// Convert the provided canvas pixel coordinates to point on viewport plane
vec::Vec3f canvasToViewport(int x, int y) {
    return vec::Vec3f{x * kViewportWidth / kCanvasWidth,
                      y * kViewportHeight / kCanvasHeight,
                      kViewportDepth};
}

/******************************************************************************
 * MAIN
 ******************************************************************************/

int main() {
    sf::RenderWindow window(sf::VideoMode(kCanvasWidth, kCanvasHeight),
                            "Raytracer View",
                            sf::Style::Titlebar | sf::Style::Close);

    // Canvas for raytracer to draw on
    Canvas canvas{kCanvasWidth, kCanvasHeight};

    // Single ray-trace pass
    for (int x = -kCanvasWidth / 2; x < (kCanvasWidth / 2); x++) {
        for (int y = -kCanvasHeight / 2; y < (kCanvasHeight / 2); y++) {
            const vec::Vec3f viewport_point = canvasToViewport(x, y);
            const sf::Color color = traceRay(kOrigin,
                                             viewport_point,
                                             1.0F, // only include objects beyond viewport
                                             std::numeric_limits<float>::max());
            canvas.put_pixel(x, y, color);
        }
    }

    // Take snapshot of all canvas pixel updates for drawing
    canvas.snapshot();

    while (window.isOpen()) {
        // Process window events
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Draw canvas to window
        window.clear();
        window.draw(canvas);
        window.display();
    }

    return EXIT_SUCCESS;
}