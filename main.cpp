#include <iostream>

#include <SFML/Graphics.hpp>

constexpr std::size_t kWindowWidth = 800U;
constexpr std::size_t kWindowHeight = 600U;

int main() {
    sf::RenderWindow window(sf::VideoMode(kWindowWidth, kWindowHeight),
                            "Raytracer View",
                            sf::Style::Titlebar | sf::Style::Close);

    // SFML image, texture, and sprite objects for easy pixel manipulation
    sf::Image image;
    image.create(kWindowWidth, kWindowHeight, sf::Color::Black);

    sf::Texture texture;
    texture.loadFromImage(image);

    sf::Sprite sprite;
    sprite.setTexture(texture);

    while (window.isOpen()) {
        // Process window events
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // TODO: Update image buffer here
        image.setPixel(200, 200, sf::Color::White);

        // Update sprite's texture from image buffer
        texture.loadFromImage(image);

        // Render sprite to window
        window.clear();
        window.draw(sprite);
        window.display();
    }

    return EXIT_SUCCESS;
}