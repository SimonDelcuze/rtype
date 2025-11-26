#include "input/InputMapper.hpp"

#include <SFML/Window/Keyboard.hpp>

std::uint16_t InputMapper::pollFlags() const
{
    std::uint16_t flags = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        flags |= static_cast<std::uint16_t>(1 << 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        flags |= static_cast<std::uint16_t>(1 << 1);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        flags |= static_cast<std::uint16_t>(1 << 2);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        flags |= static_cast<std::uint16_t>(1 << 3);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
        flags |= static_cast<std::uint16_t>(1 << 4);
    return flags;
}
