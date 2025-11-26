#include "input/InputSystem.hpp"

#include "components/TransformComponent.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <iostream>

InputSystem::InputSystem(InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter, float& posX, float& posY)
    : buffer_(&buffer), mapper_(&mapper), sequenceCounter_(&sequenceCounter), posX_(&posX), posY_(&posY)
{
}

void InputSystem::initialize() {}

void InputSystem::update(Registry& registry, float)
{
    (void)registry;
    auto flags = mapper_->pollFlags();
    if (flags == 0) {
        return;
    }

    float angle = 0.0F;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        angle = 180.0F;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        angle = 0.0F;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        angle = 270.0F;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        angle = 90.0F;
    }

    InputCommand cmd = buildCommand(flags, angle);
    buffer_->push(cmd);
}

void InputSystem::cleanup() {}

InputCommand InputSystem::buildCommand(std::uint16_t flags, float angle)
{
    InputCommand cmd{};
    cmd.flags      = flags;
    cmd.sequenceId = nextSequence();
    cmd.posX       = *posX_;
    cmd.posY       = *posY_;
    cmd.angle      = angle;
    return cmd;
}

std::uint32_t InputSystem::nextSequence()
{
    std::uint32_t seq = *sequenceCounter_;
    ++(*sequenceCounter_);
    return seq;
}
