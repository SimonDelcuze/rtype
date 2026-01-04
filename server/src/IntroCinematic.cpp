#include "server/IntroCinematic.hpp"

#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    float smoothStep(float t)
    {
        t = std::clamp(t, 0.0F, 1.0F);
        return t * t * (3.0F - 2.0F * t);
    }

    float lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }
} // namespace

void IntroCinematic::start(const std::map<std::uint32_t, EntityId>& players, Registry& registry)
{
    starts_.clear();
    for (const auto& entry : players) {
        EntityId id = entry.second;
        if (!registry.isAlive(id) || !registry.has<TransformComponent>(id)) {
            continue;
        }
        const auto& t = registry.get<TransformComponent>(id);
        starts_[id]   = PlayerStart{t.x, t.y};
    }
    elapsed_ = 0.0F;
    active_  = !starts_.empty();
}

void IntroCinematic::reset()
{
    starts_.clear();
    elapsed_ = 0.0F;
    active_  = false;
}

bool IntroCinematic::active() const
{
    return active_;
}

void IntroCinematic::update(Registry& registry, const std::map<std::uint32_t, EntityId>& players, float deltaTime)
{
    if (!active_) {
        return;
    }

    for (const auto& entry : players) {
        EntityId id = entry.second;
        if (starts_.contains(id) || !registry.isAlive(id) || !registry.has<TransformComponent>(id)) {
            continue;
        }
        const auto& t = registry.get<TransformComponent>(id);
        starts_[id]   = PlayerStart{t.x, t.y};
    }

    elapsed_ += deltaTime;

    const float forwardDuration = kIntroCinematicConfig.forwardDuration;
    const float backDuration    = kIntroCinematicConfig.backDuration;
    const float settleDuration  = kIntroCinematicConfig.settleDuration;
    const float totalDuration   = kIntroCinematicConfig.totalDuration();

    float offset = 0.0F;
    if (elapsed_ < forwardDuration) {
        float t = forwardDuration > 0.0F ? elapsed_ / forwardDuration : 1.0F;
        offset  = lerp(0.0F, kIntroCinematicConfig.forwardDistance, smoothStep(t));
    } else if (elapsed_ < forwardDuration + backDuration) {
        float t = backDuration > 0.0F ? (elapsed_ - forwardDuration) / backDuration : 1.0F;
        offset  = lerp(kIntroCinematicConfig.forwardDistance, 0.0F, smoothStep(t));
    }

    for (const auto& entry : starts_) {
        EntityId id = entry.first;
        if (!registry.isAlive(id) || !registry.has<TransformComponent>(id)) {
            continue;
        }
        auto& t = registry.get<TransformComponent>(id);
        t.x     = entry.second.x + offset;
        t.y     = entry.second.y;
        if (registry.has<VelocityComponent>(id)) {
            auto& v = registry.get<VelocityComponent>(id);
            v.vx    = 0.0F;
            v.vy    = 0.0F;
        }
    }

    if (elapsed_ >= forwardDuration + backDuration + settleDuration || elapsed_ >= totalDuration) {
        active_  = false;
        elapsed_ = totalDuration;
        starts_.clear();
    }
}
