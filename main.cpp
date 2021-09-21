// Standard lib
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// SFML
#include "SFML/Graphics.hpp"
#include "SFML/System.hpp"

// Vec
#include "vec.hpp"

// Constants
constexpr vec::Vec3f kOrigin{0.0F, 0.0F, 0.0F};

constexpr int kCanvasWidth = 400U;
constexpr int kCanvasHeight = 300U;

constexpr float kViewportWidth = 1.0F;
constexpr float kViewportHeight = 0.75F;
constexpr float kViewportDepth = 1.0F;

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
};

// Scene spheres
const std::array<Sphere, 3> kSceneSpheres {{
    {
        .center = vec::Vec3f{0.0F, -1.0F, 3.0F},
        .radius = 1.0F,
        .color = sf::Color::Red,
        .specularity = 500,
    },
    {
        .center = vec::Vec3f{2.0F, 0.0F, 4.0F},
        .radius = 1.0F,
        .color = sf::Color::Blue,
        .specularity = 500,
    },
    {
        .center = vec::Vec3f{-2.0F, 0.0F, 4.0F},
        .radius = 1.0F,
        .color = sf::Color::Green,
        .specularity = 10,
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
            .type = Type::kPoint,
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

// Compute basic lighting intensity at the specified point and surface normal
float computeLighting(vec::Vec3f point, vec::Vec3f normal, vec::Vec3f ray, float specularity) {
    float intensity = 0.0F;

    for (const Light& light : kSceneLights) {
        if (light.type == Light::Type::kAmbient) {
            // Simply add ambient light intensity
            intensity += light.intensity;
        } else {
            // Get light direction
            const vec::Vec3f direction = (light.type == Light::Type::kDirectional) ?
                    light.direction :         // use direction directly
                    (light.position - point); // calculate direction from points

            // Diffuse lighting
            // TODO: have light intensity decrease following the inverse square law?
            const float normal_dot_direction = dot(normal, direction);
            // Check if light source is actually illuminating the point
            if (normal_dot_direction > 0.0F) {
                // Calculate and add light intensity according to angle of incidence
                intensity += (light.intensity * normal_dot_direction) /
                        (normal.euclidean() * direction.euclidean());
            }

            // Specular lighting
            // Non-physical model: intensity is cos(alpha)^specularity, where alpha is the angle
            // between the reflection vector and the negative of the ray vector we're tracing
            if (specularity > 0.0F) {
                vec::Vec3f reflection = 2.0F * normal * dot(normal, direction) - direction;
                const float reflection_dot_ray = dot(reflection, -ray);
                if (reflection_dot_ray > 0.0F) {
                    const float reflection_ray_angle = reflection_dot_ray /
                            (reflection.euclidean() * ray.euclidean());
                    intensity += light.intensity * std::pow(reflection_ray_angle, specularity);
                }
            }
        }
    }

    return intensity;
}

// Get sphere intersect points
using RaySphereIntersect = std::pair<std::optional<float>, std::optional<float>>;
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
        return std::make_pair(std::nullopt, std::nullopt);
    }

    const float disc_root = std::sqrt(discriminant);
    RaySphereIntersect t;
    t.first = (-b + disc_root) / (2*a);
    t.second = (-b - disc_root) / (2*a);

    return t;
}

// Trace ray from first point toward second point over the provided independent
// variable range. Return the color of the first object the ray collides with.
sf::Color traceRay(const vec::Vec3f& ray_point,
                   const vec::Vec3f& ray_vector,
                   float t_min,
                   float t_max) {
    // Closest sphere intersect data
    float closest_t = std::numeric_limits<float>::infinity();
    std::optional<Sphere> closest_sphere;

    for (const Sphere& sphere : kSceneSpheres) {
        // Get intersect point(s) if any
        const RaySphereIntersect t = calcRaySphereIntersect(ray_point, ray_vector, sphere);

        // Check first potential intersect
        if (t.first) {
            const float t0 = *(t.first);
            if ((t0 > t_min) && (t0 < t_max) && t0 < closest_t) {
                closest_t = t0;
                closest_sphere = sphere;
            }
        }
        // Check second potential intersect
        if (t.second) {
            const float t1 = *(t.second);
            if ((t1 > t_min) && (t1 < t_max) && t1 < closest_t) {
                closest_t = t1;
                closest_sphere = sphere;
            }
        }
    }

    if (closest_sphere) {
        const vec::Vec3f intersect_point = ray_point + closest_t * ray_vector;
        const vec::Vec3f normal = (intersect_point - closest_sphere->center).normalize();
        return scaleColor(closest_sphere->color, computeLighting(intersect_point,
                                                                 normal,
                                                                 ray_vector,
                                                                 closest_sphere->specularity));
    } else {
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
    window.setVerticalSyncEnabled(true);

    // Clock object for tracking time elapsed between frames
    sf::Clock clock;

    // Canvas for raytracer to draw on
    Canvas canvas{kCanvasWidth, kCanvasHeight};

    // Framerate text
    sf::Text text_framerate;
    sf::Font font;
    font.loadFromFile("../OpenSans-Regular.ttf");
    text_framerate.setFont(font);

    // Camera position
    vec::Vec3f camera_point = kOrigin;

    while (window.isOpen()) {
        // Process window events
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Get elapsed time (TODO: also display framerate, with averaging)
        const sf::Time elapsed = clock.restart();

        // Get camera lateral velocity from keypress(es)
        float lateral_velocity = 0.0F;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            lateral_velocity -= 0.5F;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            lateral_velocity += 0.5F;
        }

        // Update camera position
        camera_point.x() += (lateral_velocity * elapsed.asSeconds());
        std::cout << "Velocity: " << lateral_velocity << std::endl;

        // Single ray-trace pass
        for (int x = -kCanvasWidth / 2; x < (kCanvasWidth / 2); x++) {
            for (int y = -kCanvasHeight / 2; y < (kCanvasHeight / 2); y++) {
                const vec::Vec3f viewport_point = canvasToViewport(x, y);
                const sf::Color color = traceRay(camera_point,
                                                 viewport_point,
                                                 1.0F, // only include objects beyond viewport
                                                 std::numeric_limits<float>::max());
                canvas.put_pixel(x, y, color);
            }
        }

        // Take snapshot of all canvas pixel updates for drawing
        canvas.snapshot();

        // Draw to window
        window.clear();
        window.draw(canvas);
        window.draw(text_framerate);
        window.display();
    }

    return EXIT_SUCCESS;
}