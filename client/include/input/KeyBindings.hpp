#pragma once

#include <SFML/Window/Keyboard.hpp>

struct KeyBindings
{
    sf::Keyboard::Key up    = sf::Keyboard::Key::Up;
    sf::Keyboard::Key down  = sf::Keyboard::Key::Down;
    sf::Keyboard::Key left  = sf::Keyboard::Key::Left;
    sf::Keyboard::Key right = sf::Keyboard::Key::Right;
    sf::Keyboard::Key fire  = sf::Keyboard::Key::Space;

    static KeyBindings defaults()
    {
        return {};
    }
};
