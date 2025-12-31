#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "../../abstraction/Common.hpp"
#include "../../abstraction/Event.hpp"

inline sf::Color toSFML(const Color& color) {
    return sf::Color(color.r, color.g, color.b, color.a);
}

inline sf::Vector2f toSFML(const Vector2f& vec) {
    return sf::Vector2f(vec.x, vec.y);
}

inline sf::Vector2u toSFML(const Vector2u& vec) {
    return sf::Vector2u(vec.x, vec.y);
}

inline sf::IntRect toSFML(const IntRect& rect) {
    return sf::IntRect({rect.left, rect.top}, {rect.width, rect.height});
}

inline Vector2f fromSFML(const sf::Vector2f& vec) {
    return {vec.x, vec.y};
}

inline FloatRect fromSFML(const sf::FloatRect& rect) {
    return {rect.position.x, rect.position.y, rect.size.x, rect.size.y};
}

Event fromSFML(const sf::Event& event);
