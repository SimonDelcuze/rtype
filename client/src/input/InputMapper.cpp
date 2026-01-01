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

void InputMapper::handleEvent(const Event& event)
{
    if (event.type == EventType::KeyPressed) {
        setKeyState(event.key.code, true);
    } else if (event.type == EventType::KeyReleased) {
        setKeyState(event.key.code, false);
    } else if (event.type == EventType::LostFocus) {
        upPressed_ = downPressed_ = leftPressed_ = rightPressed_ = firePressed_ = false;
    }
}

void InputMapper::setKeyState(KeyCode key, bool pressed)
{
    if (key == bindings_.up)
        upPressed_ = pressed;
    if (key == bindings_.down)
        downPressed_ = pressed;
    if (key == bindings_.left)
        leftPressed_ = pressed;
    if (key == bindings_.right)
        rightPressed_ = pressed;
    if (key == bindings_.fire)
        firePressed_ = pressed;
}

void InputMapper::setBindings(const KeyBindings& bindings)
{
    bindings_ = bindings;
}

KeyBindings InputMapper::getBindings() const
{
    return bindings_;
}
