// Standard lib
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
constexpr int kCanvasHeight = 800U;

constexpr float kViewportWidth = 1.0F;
constexpr float kViewportHeight = 1.0F;
constexpr float kViewportDepth = 1.0F;

// Simple canvas abstraction to facilitate drawing individual pixels to the screen
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

// Basic sphere scene object
struct Sphere {
    vec::Vec3f center;
    float radius;
    sf::Color color;
};

// Scene objects (currently just spheres)
// Note: can't be constexpr due to sf::Color implementation
// TODO: load scene data from a file at runtime
const std::array<Sphere, 3> kSceneSpheres = {{
    {vec::Vec3f{0.0F, -1.0F, 3.0F}, 1.0F, sf::Color::Red},
    {vec::Vec3f{2.0F, 0.0F, 4.0F}, 1.0F, sf::Color::Blue},
    {vec::Vec3f{-2.0F, 0.0F, 4.0F}, 1.0F, sf::Color::Green},
}};

// Convert the provided canvas pixel coordinates to point on viewport plane
vec::Vec3f canvas_to_viewport(int x, int y) {
    return vec::Vec3f{x * kViewportWidth / kCanvasWidth,
                      y * kViewportHeight / kCanvasHeight,
                      kViewportDepth};
}

// Get sphere intersect points
using RaySphereIntersect = std::pair<std::optional<float>, std::optional<float>>;
RaySphereIntersect calc_ray_sphere_intersect(const vec::Vec3f& p0,
                                             const vec::Vec3f& p1,
                                             const Sphere& sphere) {
    const float r = sphere.radius;
    const vec::Vec3f c_p0 = p0 - sphere.center;

    const float a = dot(p1, p1);
    const float b = dot(c_p0, p1) * 2.0F;
    const float c = dot(c_p0, c_p0) - (r*r);

    const float discriminant = (b*b) - (4*a*c);
    if (discriminant < 0.0F) {
        // No intersect point
        return std::make_pair(std::nullopt, std::nullopt);
    }

    // TODO: Is it worth trying to handle the discriminant == 0 (approx) case separately?

    const float disc_root = std::sqrt(discriminant);
    RaySphereIntersect t;
    t.first = (-b + disc_root) / (2*a);
    t.second = (-b - disc_root) / (2*a);

    return t;
}

// Trace ray from first point toward second point over the provided independent
// variable range. Return the color of the first object the ray collides with.
sf::Color trace_ray(const vec::Vec3f& p0, const vec::Vec3f& p1, float t_min, float t_max) {
    // Closest sphere intersect data
    float closest_t = std::numeric_limits<float>::infinity();
    std::optional<Sphere> closest_sphere;

    // TODO: find a better way to access scene data
    for (const Sphere& sphere : kSceneSpheres) {
        // Get intersect point(s)
        const RaySphereIntersect t = calc_ray_sphere_intersect(p0, p1, sphere);

        // Check first potential intersect
        if (t.first.has_value()) {
            const float t0 = t.first.value();
            if ((t0 > t_min) && (t0 < t_max) && t0 < closest_t) {
                closest_t = t0;
                closest_sphere = sphere;
            }
        }
        // Check second potential intersect
        if (t.second.has_value()) {
            const float t1 = t.second.value();
            if ((t1 > t_min) && (t1 < t_max) && t1 < closest_t) {
                closest_t = t1;
                closest_sphere = sphere;
            }
        }
    }

    // Return closest sphere color, or white if no intersect
    return closest_sphere ? closest_sphere.value().color : sf::Color::White;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(kCanvasWidth, kCanvasHeight),
                            "Raytracer View",
                            sf::Style::Titlebar | sf::Style::Close);

    // Canvas for raytracer to draw on
    Canvas canvas{kCanvasWidth, kCanvasHeight};

    // Single ray-trace pass
    for (int x = -kCanvasWidth / 2; x < (kCanvasWidth / 2); x++) {
        for (int y = -kCanvasHeight / 2; y < (kCanvasHeight / 2); y++) {
            const vec::Vec3f viewport_point = canvas_to_viewport(x, y);
            const sf::Color color = trace_ray(kOrigin,
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