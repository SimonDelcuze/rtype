#include "systems/AnimationSystem.hpp"

void AnimationSystem::setSpriteRectCallback(SpriteRectCallback callback)
{
    spriteRectCallback_ = std::move(callback);
}

void AnimationSystem::advanceFrame(AnimationComponent& anim)
{
    if (anim.frameIndices.empty()) {
        return;
    }

    std::uint32_t frameCount = static_cast<std::uint32_t>(anim.frameIndices.size());

    switch (anim.direction) {
        case AnimationDirection::Forward:
            anim.currentFrame++;
            if (anim.currentFrame >= frameCount) {
                if (anim.loop) {
                    anim.currentFrame = 0;
                } else {
                    anim.currentFrame = frameCount - 1;
                    anim.finished     = true;
                    anim.playing      = false;
                }
            }
            break;

        case AnimationDirection::Reverse:
            if (anim.currentFrame == 0) {
                if (anim.loop) {
                    anim.currentFrame = frameCount - 1;
                } else {
                    anim.finished = true;
                    anim.playing  = false;
                }
            } else {
                anim.currentFrame--;
            }
            break;

        case AnimationDirection::PingPong:
            if (anim.pingPongReverse) {
                if (anim.currentFrame == 0) {
                    anim.pingPongReverse = false;
                    if (!anim.loop) {
                        anim.finished = true;
                        anim.playing  = false;
                    } else {
                        anim.currentFrame++;
                    }
                } else {
                    anim.currentFrame--;
                }
            } else {
                anim.currentFrame++;
                if (anim.currentFrame >= frameCount - 1) {
                    anim.pingPongReverse = true;
                }
            }
            break;
    }
}

void AnimationSystem::updateSpriteRect(EntityId entity, const AnimationComponent& anim)
{
    if (!spriteRectCallback_ || anim.frameWidth == 0 || anim.frameHeight == 0) {
        return;
    }

    std::uint32_t frameIndex = anim.getCurrentFrameIndex();
    std::uint32_t col        = frameIndex % anim.columns;
    std::uint32_t row        = frameIndex / anim.columns;
    std::uint32_t x          = col * anim.frameWidth;
    std::uint32_t y          = row * anim.frameHeight;

    spriteRectCallback_(entity, x, y, anim.frameWidth, anim.frameHeight);
}

void AnimationSystem::update(Registry& registry, float deltaTime)
{
    for (EntityId entity = 0; entity < registry.entityCount(); ++entity) {
        if (!registry.isAlive(entity) || !registry.has<AnimationComponent>(entity)) {
            continue;
        }

        AnimationComponent& anim = registry.get<AnimationComponent>(entity);

        if (!anim.playing || anim.finished || anim.frameIndices.empty()) {
            continue;
        }

        anim.elapsedTime += deltaTime;

        while (anim.elapsedTime >= anim.frameTime && anim.playing) {
            anim.elapsedTime -= anim.frameTime;
            advanceFrame(anim);
            updateSpriteRect(entity, anim);
        }
    }
}
