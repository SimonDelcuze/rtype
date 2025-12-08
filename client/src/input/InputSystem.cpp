#include "input/InputSystem.hpp"

#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <cmath>

namespace
{
    constexpr float kMoveSpeed = 250.0F;
}

InputSystem::InputSystem(InputBuffer& buffer, InputMapper& mapper, std::uint32_t& sequenceCounter, float& posX,
                         float& posY)
    : buffer_(&buffer), mapper_(&mapper), sequenceCounter_(&sequenceCounter), posX_(&posX), posY_(&posY)
{}

void InputSystem::initialize() {}

void InputSystem::update(Registry& registry, float deltaTime)
{
    if (!positionInitialized_) {
        auto view = registry.view<TransformComponent, TagComponent>();
        for (auto id : view) {
            const auto& tag = registry.get<TagComponent>(id);
            if (!tag.hasTag(EntityTag::Player))
                continue;
            const auto& t        = registry.get<TransformComponent>(id);
            *posX_               = t.x;
            *posY_               = t.y;
            positionInitialized_ = true;
            break;
        }
    }

    fireElapsed_ += deltaTime;

    auto flags   = mapper_->pollFlags();
    bool changed = flags != lastFlags_;

    const bool firePressed = (flags & InputMapper::FireFlag) != 0;
    bool fireAllowed       = firePressed && (changed || fireElapsed_ >= fireCooldown_);
    if (!fireAllowed)
        flags &= static_cast<std::uint16_t>(~InputMapper::FireFlag);
    if (fireAllowed)
        fireElapsed_ = 0.0F;

    bool inactive = flags == 0;

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

    float dx = 0.0F;
    float dy = 0.0F;
    if (left)
        dx -= 1.0F;
    if (right)
        dx += 1.0F;
    if (up)
        dy -= 1.0F;
    if (down)
        dy += 1.0F;
    if (dx != 0.0F || dy != 0.0F) {
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.0F) {
            dx /= len;
            dy /= len;
        }
        *posX_ += dx * kMoveSpeed * deltaTime;
        *posY_ += dy * kMoveSpeed * deltaTime;
        auto view = registry.view<TransformComponent, TagComponent>();
        for (auto id : view) {
            const auto& tag = registry.get<TagComponent>(id);
            if (!tag.hasTag(EntityTag::Player))
                continue;
            auto& t = registry.get<TransformComponent>(id);
            t.x     = *posX_;
            t.y     = *posY_;
            break;
        }
    }
    if (inactive && changed) {
        auto view = registry.view<TransformComponent, TagComponent, VelocityComponent>();
        for (auto id : view) {
            const auto& tag = registry.get<TagComponent>(id);
            if (!tag.hasTag(EntityTag::Player))
                continue;
            auto& vel = registry.get<VelocityComponent>(id);
            vel.vx    = 0.0F;
            vel.vy    = 0.0F;
            break;
        }
    }

    if (inactive && !changed) {
        return;
    }

    InputCommand cmd = buildCommand(flags, angle);
    buffer_->push(cmd);
    lastFlags_ = flags;
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
