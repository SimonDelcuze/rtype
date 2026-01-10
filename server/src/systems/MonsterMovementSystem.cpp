#include "systems/MonsterMovementSystem.hpp"

#include <cmath>
#include <limits>

namespace
{
    constexpr float kTwoPi = 6.28318530717958647692F;
}

void MonsterMovementSystem::update(Registry& registry, float deltaTime) const
{
    for (EntityId id : registry.view<MovementComponent, VelocityComponent, TransformComponent>()) {
        if (!registry.isAlive(id))
            continue;
        auto& move = registry.get<MovementComponent>(id);
        auto& vel  = registry.get<VelocityComponent>(id);
        const auto& selfTransform = registry.get<TransformComponent>(id);
        move.time += deltaTime;
        switch (move.pattern) {
            case MovementPattern::Linear:
                vel.vx = -move.speed;
                vel.vy = 0.0F;
                break;
            case MovementPattern::Zigzag: {
                vel.vx = -move.speed;
                if (move.frequency <= 0.0F || !std::isfinite(move.frequency)) {
                    vel.vy = 0.0F;
                    break;
                }
                float cycle = std::fmod(move.time * move.frequency, 1.0F);
                if (cycle < 0.0F)
                    cycle += 1.0F;
                vel.vy = (cycle < 0.5F ? move.amplitude : -move.amplitude);
                break;
            }
            case MovementPattern::Sine: {
                vel.vx = -move.speed;
                if (move.frequency <= 0.0F || !std::isfinite(move.frequency) || !std::isfinite(move.amplitude)) {
                    vel.vy = 0.0F;
                    break;
                }
                float arg = move.phase + kTwoPi * move.frequency * move.time;
                vel.vy    = move.amplitude * std::sin(arg);
                break;
            }
            case MovementPattern::FollowPlayer: {
                float bestDist2 = std::numeric_limits<float>::max();
                float bestDx    = 0.0F;
                float bestDy    = 0.0F;
                for (EntityId playerId : registry.view<TransformComponent, TagComponent>()) {
                    if (!registry.isAlive(playerId))
                        continue;
                    const auto& tag = registry.get<TagComponent>(playerId);
                    if (!tag.hasTag(EntityTag::Player))
                        continue;
                    const auto& p = registry.get<TransformComponent>(playerId);
                    float dx      = p.x - selfTransform.x;
                    float dy      = p.y - selfTransform.y;
                    float dist2   = dx * dx + dy * dy;
                    if (dist2 < bestDist2) {
                        bestDist2 = dist2;
                        bestDx    = dx;
                        bestDy    = dy;
                    }
                }
                if (bestDist2 > 0.0F && bestDist2 < std::numeric_limits<float>::max()) {
                    float invLen = 1.0F / std::sqrt(bestDist2);
                    vel.vx       = bestDx * invLen * move.speed;
                    vel.vy       = bestDy * invLen * move.speed;
                } else {
                    vel.vx = -move.speed;
                    vel.vy = 0.0F;
                }
                break;
            }
            default:
                vel.vx = 0.0F;
                vel.vy = 0.0F;
                break;
        }
        if (!std::isfinite(vel.vx))
            vel.vx = 0.0F;
        if (!std::isfinite(vel.vy))
            vel.vy = 0.0F;
    }
}
