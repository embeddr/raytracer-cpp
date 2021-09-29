// Standard lib
#include <limits>
#include <iostream>
#include <thread>

// SFML
#include "SFML/Graphics.hpp"

// Vec
#include "vec.hpp"
#include "transform.hpp"

// Project
#include "canvas.hpp"
#include "raytrace.hpp"
#include "scene.hpp"

constexpr unsigned int kNumThreads = 8U;

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

// Update the canvas by performing the raytracing algorithm across all pixels
void update_canvas(Canvas& canvas, const vec::Transform3f& camera) {
    // Helper to process a segment of the canvas
    auto update_cols = [&](int segment, int num_segments) mutable {
        // Determine begin/end columns for this segment
        const int x_begin = segment * (kCanvasWidth/num_segments) - (kCanvasWidth/2);
        const int x_end = (segment == (num_segments-1)) ?
                (kCanvasWidth / 2) : // Final segment, process to final column
                (segment+1) * (kCanvasWidth/num_segments) - (kCanvasWidth/2);

        // Update columns
        for (int x = x_begin; x < x_end; x++) {
            for (int y = -(kCanvasHeight / 2); y < (kCanvasHeight / 2); y++) {
                const vec::Vec3f viewport_vector =
                        canvas_to_viewport(x, y) * camera.get_linear_transform();
                const Ray ray = {camera.get_translation(), viewport_vector};
                const sf::Color color = trace_ray(ray,
                                                  kViewportDepth,
                                                  std::numeric_limits<float>::max());
                canvas.put_pixel(x, y, color);
            }
        }
    };

    // Spawn threads to update all canvas columns
    std::array<std::thread, kNumThreads> threads;
    unsigned int segment = 0U;
    for (auto& thread : threads) {
        thread = std::thread{update_cols, segment++, kNumThreads};
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Take snapshot of all canvas pixel updates for drawing
    canvas.snapshot();
}

int main() {
    // SFML window to display
    sf::RenderWindow window(sf::VideoMode(kCanvasWidth, kCanvasHeight),
                            "Raytracer View",
                            sf::Style::Titlebar | sf::Style::Close);

    // Canvas for raytracer to draw pixels on
    Canvas canvas{kCanvasWidth, kCanvasHeight};

    // Get first camera from scene
    if (kSceneCameras.empty()) {
        std::cerr << "No cameras defined in scene!" << std::endl;
        exit(1);
    }
    auto camera_iter = kSceneCameras.cbegin();

    // Perform single raytracing pass
    update_canvas(canvas, camera_iter->transform);

    while (window.isOpen()) {
        // Process window events
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Move to next camera position on space keypress
        static bool space_pressed_prev = false;
        bool space_pressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
        if (space_pressed && !space_pressed_prev) {
            if (++camera_iter == kSceneCameras.cend()) {
                camera_iter = kSceneCameras.cbegin();
            }
            update_canvas(canvas, camera_iter->transform);
        }
        space_pressed_prev = space_pressed;

        // Draw canvas to window
        window.clear();
        window.draw(canvas);
        window.display();
    }

    return EXIT_SUCCESS;
}