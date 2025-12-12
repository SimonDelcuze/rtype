#include "input/InputMapper.hpp"

std::uint16_t InputMapper::pollFlags() const
{
    std::uint16_t flags = 0;
    if (upPressed_)
        flags |= InputMapper::UpFlag;
    if (downPressed_)
        flags |= InputMapper::DownFlag;
    if (leftPressed_)
        flags |= InputMapper::LeftFlag;
    if (rightPressed_)
        flags |= InputMapper::RightFlag;
    if (firePressed_)
        flags |= InputMapper::FireFlag;
    return flags;
}

void InputMapper::handleEvent(const sf::Event& event)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        setKeyState(key->code, true);
    } else if (const auto* key = event.getIf<sf::Event::KeyReleased>()) {
        setKeyState(key->code, false);
    } else if (event.is<sf::Event::FocusLost>()) {
        upPressed_ = downPressed_ = leftPressed_ = rightPressed_ = firePressed_ = false;
    }
}

void InputMapper::setKeyState(sf::Keyboard::Key key, bool pressed)
{
    switch (key) {
        case sf::Keyboard::Key::Up:
            upPressed_ = pressed;
            break;
        case sf::Keyboard::Key::Down:
            downPressed_ = pressed;
            break;
        case sf::Keyboard::Key::Left:
            leftPressed_ = pressed;
            break;
        case sf::Keyboard::Key::Right:
            rightPressed_ = pressed;
            break;
        case sf::Keyboard::Key::Space:
            firePressed_ = pressed;
            break;
        default:
            break;
    }
}
