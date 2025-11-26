#include "input/InputMapper.hpp"

#include <SFML/Window/Keyboard.hpp>

std::uint16_t InputMapper::pollFlags() const
{
    std::uint16_t flags = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        flags |= InputMapper::UpFlag;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        flags |= InputMapper::DownFlag;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        flags |= InputMapper::LeftFlag;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        flags |= InputMapper::RightFlag;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
        flags |= InputMapper::FireFlag;
    return flags;
}
