// Standard lib
#include <limits>
#include <iostream>

// SFML
#include "SFML/Graphics.hpp"

// Vec
#include "vec.hpp"

// Project
#include "canvas.hpp"
#include "raytrace.hpp"

// Constants
constexpr vec::Vec3f kOrigin{0.0F, 0.0F, 0.0F};

constexpr int kCanvasWidth = 800U;
constexpr int kCanvasHeight = 600U;

constexpr float kViewportWidth = 1.0F;
constexpr float kViewportHeight = 0.75F;
constexpr float kViewportDepth = 0.75F;

// Convert the provided canvas pixel coordinates to point on viewport plane
vec::Vec3f canvas_to_viewport(int x, int y) {
    return vec::Vec3f{x * kViewportWidth / kCanvasWidth,
                      y * kViewportHeight / kCanvasHeight,
                      kViewportDepth};
}

// Update the canvas by performing the raytracing algorithim for all pixels
// TODO: Parallelize this function
void update_canvas(Canvas& canvas) {
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
}

int main() {
    sf::RenderWindow window(sf::VideoMode(kCanvasWidth, kCanvasHeight),
                            "Raytracer View",
                            sf::Style::Titlebar | sf::Style::Close);

    // Canvas for raytracer to draw on
    Canvas canvas{kCanvasWidth, kCanvasHeight};

    // Perform single raytracing pass on canvas
    update_canvas(canvas);

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