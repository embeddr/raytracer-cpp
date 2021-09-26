// Simple canvas abstraction to facilitate drawing individual pixels to the window.
//
// The book "Computer Graphics From Scratch" by Gabriel Gambetta was used as a starting point for
// this project. The book walks the reader through the basics of raytracers and rasterizers, and
// assumes the existence of a hypothetical "put_pixel()" function that simply allows drawing a given
// color to a canvas at specified x and y coordinates.
//
// This class provides that functionality in practice by using a few SFML objects and minimal
// extra logic. It's not highly performant, but serves my needs for a non-realtime raytracer.

#pragma once

#include <SFML/Graphics.hpp>

// Drawable canvas with an interface to place individual pixels
class Canvas : public sf::Drawable {
public:
    // Construct a canvas of specified dimensions and color (default black)
    Canvas(std::size_t width, std::size_t height, sf::Color color=sf::Color::Black) :
        width_(width),
        height_(height)
    {
        image_.create(width_, height_, color);
        texture_.loadFromImage(image_);
        sprite_.setTexture(texture_);
    }

    // Place a pixel of the specified color at the specified coordinates
    // Origin is at center of canvas, positive x is right, positive y is up
    void put_pixel(int x, int y, sf::Color color) {
        image_.setPixel((width_ / 2) + x, (height_ / 2) - y - 1, color);
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

    std::size_t width_;
    std::size_t height_;
};

