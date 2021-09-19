#include <iostream>

#include <SFML/Graphics.hpp>

constexpr std::size_t kWindowWidth = 800U;
constexpr std::size_t kWindowHeight = 600U;

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

int main() {
    sf::RenderWindow window(sf::VideoMode(kWindowWidth, kWindowHeight),
                            "Raytracer View",
                            sf::Style::Titlebar | sf::Style::Close);

    // Canvas for raytracer to draw on
    Canvas canvas{kWindowWidth, kWindowHeight};

    while (window.isOpen()) {
        // Process window events
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // TODO: Update canvas pixels here
        canvas.put_pixel(0, 0, sf::Color::White);

        // Take snapshot of all canvas pixel updates for drawing
        canvas.snapshot();

        // Draw canvas to window
        window.clear();
        window.draw(canvas);
        window.display();
    }

    return EXIT_SUCCESS;
}