#include "systems/FollowerFacingSystem.hpp"

#include "components/TagComponent.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

FollowerFacingSystem::FollowerFacingSystem(AnimationRegistry& animations, AnimationLabels& labels)
    : animations_(&animations), labels_(&labels)
{}

void FollowerFacingSystem::applyClipFrame(Registry& registry, EntityId id, const AnimationClip& clip,
                                          std::uint32_t frameIndex) const
{
    if (!registry.has<SpriteComponent>(id) || !registry.has<AnimationComponent>(id)) {
        return;
    }
    auto& sprite = registry.get<SpriteComponent>(id);
    auto& anim   = registry.get<AnimationComponent>(id);

    anim.frameIndices.clear();
    anim.frameIndices.reserve(clip.frames.size());
    for (std::uint32_t i = 0; i < clip.frames.size(); ++i) {
        anim.frameIndices.push_back(i);
    }
    anim.frameTime       = clip.frameTime;
    anim.currentFrame    = std::min(frameIndex, static_cast<std::uint32_t>(clip.frames.size() - 1));
    anim.elapsedTime     = 0.0F;
    anim.loop            = clip.loop;
    anim.playing         = true;
    anim.finished        = false;
    anim.pingPongReverse = false;
    sprite.customFrames.clear();
    sprite.customFrames.reserve(clip.frames.size());
    for (const auto& f : clip.frames) {
        sprite.customFrames.emplace_back(f.x, f.y, f.width, f.height);
    }
    sprite.setFrame(anim.currentFrame);
}

void FollowerFacingSystem::update(Registry& registry, float)
{
    for (EntityId id : registry.view<TransformComponent, VelocityComponent, SpriteComponent, RenderTypeComponent>()) {
        if (!registry.isAlive(id))
            continue;

        const auto& renderType = registry.get<RenderTypeComponent>(id);
        if (renderType.typeId != 23) {
            continue;
        }

        const std::string* walkLeftLabel  = labels_->labelFor("follower", "walk_left");
        const std::string* walkRightLabel = labels_->labelFor("follower", "walk_right");
        if (walkLeftLabel == nullptr && walkRightLabel == nullptr) {
            continue;
        }

        auto& transform = registry.get<TransformComponent>(id);
        const auto& vel = registry.get<VelocityComponent>(id);

        if (!registry.has<FollowerFacingComponent>(id)) {
            registry.emplace<FollowerFacingComponent>(id);
        }
        auto& state = registry.get<FollowerFacingComponent>(id);

        bool desiredRight                         = vel.vx >= 0.0F;
        float bestDist2                           = std::numeric_limits<float>::max();
        const TransformComponent* targetTransform = nullptr;
        for (EntityId pid : registry.view<TransformComponent, TagComponent>()) {
            if (!registry.isAlive(pid))
                continue;
            const auto& tag = registry.get<TagComponent>(pid);
            if (!tag.hasTag(EntityTag::Player))
                continue;
            const auto& p = registry.get<TransformComponent>(pid);
            float dx      = p.x - transform.x;
            float dy      = p.y - transform.y;
            float dist2   = dx * dx + dy * dy;
            if (dist2 < bestDist2) {
                bestDist2       = dist2;
                targetTransform = &p;
            }
        }
        if (targetTransform != nullptr) {
            desiredRight = (targetTransform->x - transform.x) >= 0.0F;
        }

        bool directionChanged = !state.initialized || (state.facingRight != desiredRight);
        if (directionChanged) {
            const AnimationClip* rightClip    = animations_->get("follower_walk_right");
            const AnimationClip* leftClip     = animations_->get("follower_walk_left");
            const AnimationClip* desiredClip  = desiredRight ? rightClip : leftClip;
            const AnimationClip* fallbackClip = desiredRight ? leftClip : rightClip;
            const AnimationClip* clip         = desiredClip != nullptr ? desiredClip : fallbackClip;
            if (clip != nullptr && !clip->frames.empty()) {
                if (!registry.has<AnimationComponent>(id)) {
                    registry.emplace<AnimationComponent>(id);
                }
                applyClipFrame(registry, id, *clip, 0);
            }
        }
        state.facingRight = desiredRight;
        state.initialized = true;
    }
}
