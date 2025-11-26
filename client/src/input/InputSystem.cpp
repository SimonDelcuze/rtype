#include "input/InputSystem.hpp"

InputSystem::InputSystem(InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter, float& posX,
                         float& posY)
    : buffer_(&buffer), mapper_(&mapper), sequenceCounter_(&sequenceCounter), posX_(&posX), posY_(&posY)
{}

void InputSystem::initialize() {}

void InputSystem::update(Registry& registry, float)
{
    (void) registry;
    auto flags = mapper_->pollFlags();
    if (flags == 0) {
        return;
    }

    const bool left  = (flags & InputMapper::LeftFlag) != 0;
    const bool right = (flags & InputMapper::RightFlag) != 0;
    const bool up    = (flags & InputMapper::UpFlag) != 0;
    const bool down  = (flags & InputMapper::DownFlag) != 0;

    float angle = 0.0F;
    if (left && up) {
        angle = 225.0F;
    } else if (left && down) {
        angle = 135.0F;
    } else if (right && up) {
        angle = 315.0F;
    } else if (right && down) {
        angle = 45.0F;
    } else if (left) {
        angle = 180.0F;
    } else if (right) {
        angle = 0.0F;
    } else if (up) {
        angle = 270.0F;
    } else if (down) {
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
