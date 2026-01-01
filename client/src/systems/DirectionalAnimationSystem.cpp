#include "systems/DirectionalAnimationSystem.hpp"

#include "components/AnimationComponent.hpp"
#include "components/DirectionalAnimationComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/VelocityComponent.hpp"

#include <algorithm>
#include <cmath>

DirectionalAnimationSystem::DirectionalAnimationSystem(AnimationRegistry& animations, AnimationLabels& labels)
    : animations_(&animations), labels_(&labels)
{}

void DirectionalAnimationSystem::applyClipFrame(Registry& registry, EntityId id, const AnimationClip& clip,
                                                std::uint32_t frameIndex)
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
    anim.playing         = false;
    anim.finished        = false;
    anim.pingPongReverse = false;
    sprite.customFrames.clear();
    sprite.customFrames.reserve(clip.frames.size());
    for (const auto& f : clip.frames) {
        sprite.customFrames.emplace_back(f.x, f.y, f.width, f.height);
    }
    sprite.setFrame(anim.currentFrame);
}

DirectionalAnimationSystem::Clips
DirectionalAnimationSystem::resolveClips(const DirectionalAnimationComponent& dirAnim) const
{
    Clips clips{};
    const std::string* idleId = labels_->labelFor(dirAnim.spriteId, dirAnim.idleLabel);
    const std::string* upId   = labels_->labelFor(dirAnim.spriteId, dirAnim.upLabel);
    const std::string* downId = labels_->labelFor(dirAnim.spriteId, dirAnim.downLabel);
    if (idleId != nullptr) {
        clips.idle = animations_->get(*idleId);
    }
    if (upId != nullptr) {
        clips.up = animations_->get(*upId);
    }
    if (downId != nullptr) {
        clips.down = animations_->get(*downId);
    }
    return clips;
}

std::pair<bool, bool> DirectionalAnimationSystem::intents(const VelocityComponent& vel, float threshold) const
{
    return {vel.vy<-threshold, vel.vy> threshold};
}

void DirectionalAnimationSystem::startIdle(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                           const Clips& clips)
{
    if (clips.idle == nullptr) {
        return;
    }
    applyClipFrame(registry, id, *clips.idle, 0);
    dirAnim.phase     = DirectionalAnimationComponent::Phase::Idle;
    dirAnim.phaseTime = 0.0F;
}

void DirectionalAnimationSystem::startUpIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                           const Clips& clips)
{
    if (clips.up == nullptr) {
        return;
    }
    applyClipFrame(registry, id, *clips.up, 0);
    dirAnim.phase     = DirectionalAnimationComponent::Phase::UpIn;
    dirAnim.phaseTime = clips.up->frameTime;
}

void DirectionalAnimationSystem::startDownIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                             const Clips& clips)
{
    if (clips.down == nullptr) {
        return;
    }
    applyClipFrame(registry, id, *clips.down, 0);
    dirAnim.phase     = DirectionalAnimationComponent::Phase::DownIn;
    dirAnim.phaseTime = clips.down->frameTime;
}

void DirectionalAnimationSystem::handleUpIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                            const Clips& clips, bool downIntent, float deltaTime)
{
    if (downIntent) {
        startDownIn(registry, id, dirAnim, clips);
        return;
    }
    dirAnim.phaseTime -= deltaTime;
    if (dirAnim.phaseTime <= 0.0F && clips.up != nullptr) {
        applyClipFrame(registry, id, *clips.up, static_cast<std::uint32_t>(clips.up->frames.size() - 1));
        dirAnim.phase = DirectionalAnimationComponent::Phase::UpHold;
    }
}

void DirectionalAnimationSystem::handleUpHold(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                              const Clips& clips, bool upIntent, bool downIntent)
{
    if (downIntent) {
        startDownIn(registry, id, dirAnim, clips);
    } else if (!upIntent && clips.up != nullptr) {
        applyClipFrame(registry, id, *clips.up, static_cast<std::uint32_t>(clips.up->frames.size() - 1));
        dirAnim.phase     = DirectionalAnimationComponent::Phase::UpOut;
        dirAnim.phaseTime = clips.up->frameTime;
    }
}

void DirectionalAnimationSystem::handleUpOut(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                             const Clips& clips, float deltaTime)
{
    dirAnim.phaseTime -= deltaTime;
    if (dirAnim.phaseTime <= 0.0F) {
        startIdle(registry, id, dirAnim, clips);
    }
}

void DirectionalAnimationSystem::handleDownIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                              const Clips& clips, bool upIntent, float deltaTime)
{
    if (upIntent) {
        startUpIn(registry, id, dirAnim, clips);
        return;
    }
    dirAnim.phaseTime -= deltaTime;
    if (dirAnim.phaseTime <= 0.0F && clips.down != nullptr) {
        applyClipFrame(registry, id, *clips.down, static_cast<std::uint32_t>(clips.down->frames.size() - 1));
        dirAnim.phase = DirectionalAnimationComponent::Phase::DownHold;
    }
}

void DirectionalAnimationSystem::handleDownHold(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                                const Clips& clips, bool downIntent, bool upIntent)
{
    if (upIntent) {
        startUpIn(registry, id, dirAnim, clips);
    } else if (!downIntent && clips.down != nullptr) {
        applyClipFrame(registry, id, *clips.down, static_cast<std::uint32_t>(clips.down->frames.size() - 1));
        dirAnim.phase     = DirectionalAnimationComponent::Phase::DownOut;
        dirAnim.phaseTime = clips.down->frameTime;
    }
}

void DirectionalAnimationSystem::handleDownOut(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                               const Clips& clips, float deltaTime)
{
    dirAnim.phaseTime -= deltaTime;
    if (dirAnim.phaseTime <= 0.0F) {
        startIdle(registry, id, dirAnim, clips);
    }
}

void DirectionalAnimationSystem::handlePhase(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim,
                                             const Clips& clips, bool upIntent, bool downIntent, float deltaTime)
{
    switch (dirAnim.phase) {
        case DirectionalAnimationComponent::Phase::Idle:
            if (upIntent) {
                startUpIn(registry, id, dirAnim, clips);
            } else if (downIntent) {
                startDownIn(registry, id, dirAnim, clips);
            } else if (dirAnim.phaseTime < 0.0F) {
                startIdle(registry, id, dirAnim, clips);
            }
            break;
        case DirectionalAnimationComponent::Phase::UpIn:
            handleUpIn(registry, id, dirAnim, clips, downIntent, deltaTime);
            break;
        case DirectionalAnimationComponent::Phase::UpHold:
            handleUpHold(registry, id, dirAnim, clips, upIntent, downIntent);
            break;
        case DirectionalAnimationComponent::Phase::UpOut:
            handleUpOut(registry, id, dirAnim, clips, deltaTime);
            break;
        case DirectionalAnimationComponent::Phase::DownIn:
            handleDownIn(registry, id, dirAnim, clips, upIntent, deltaTime);
            break;
        case DirectionalAnimationComponent::Phase::DownHold:
            handleDownHold(registry, id, dirAnim, clips, downIntent, upIntent);
            break;
        case DirectionalAnimationComponent::Phase::DownOut:
            handleDownOut(registry, id, dirAnim, clips, deltaTime);
            break;
    }
}

void DirectionalAnimationSystem::update(Registry& registry, float deltaTime)
{
    for (EntityId id :
         registry.view<DirectionalAnimationComponent, AnimationComponent, SpriteComponent, VelocityComponent>()) {
        if (!registry.isAlive(id))
            continue;

        auto& dirAnim   = registry.get<DirectionalAnimationComponent>(id);
        const auto& vel = registry.get<VelocityComponent>(id);

        Clips clips = resolveClips(dirAnim);
        if (clips.idle == nullptr || clips.up == nullptr || clips.down == nullptr) {
            continue;
        }

        auto [upIntent, downIntent] = intents(vel, dirAnim.threshold);
        handlePhase(registry, id, dirAnim, clips, upIntent, downIntent, deltaTime);
    }
}
