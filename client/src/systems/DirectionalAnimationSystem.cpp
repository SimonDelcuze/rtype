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
        sprite.customFrames.emplace_back(sf::Vector2i{f.x, f.y}, sf::Vector2i{f.width, f.height});
    }
    sprite.setFrame(anim.currentFrame);
}

void DirectionalAnimationSystem::update(Registry& registry, float deltaTime)
{
    for (EntityId id :
         registry.view<DirectionalAnimationComponent, AnimationComponent, SpriteComponent, VelocityComponent>()) {
        if (!registry.isAlive(id))
            continue;

        auto& dirAnim   = registry.get<DirectionalAnimationComponent>(id);
        const auto& vel = registry.get<VelocityComponent>(id);

        const std::string* idleId = labels_->labelFor(dirAnim.spriteId, dirAnim.idleLabel);
        const std::string* upId   = labels_->labelFor(dirAnim.spriteId, dirAnim.upLabel);
        const std::string* downId = labels_->labelFor(dirAnim.spriteId, dirAnim.downLabel);
        if (idleId == nullptr || upId == nullptr || downId == nullptr) {
            continue;
        }
        const AnimationClip* idleClip = animations_->get(*idleId);
        const AnimationClip* upClip   = animations_->get(*upId);
        const AnimationClip* downClip = animations_->get(*downId);
        if (idleClip == nullptr || upClip == nullptr || downClip == nullptr) {
            continue;
        }

        bool upIntent   = vel.vy < -dirAnim.threshold;
        bool downIntent = vel.vy > dirAnim.threshold;

        auto startUpIn = [&]() {
            applyClipFrame(registry, id, *upClip, 0);
            dirAnim.phase     = DirectionalAnimationComponent::Phase::UpIn;
            dirAnim.phaseTime = upClip->frameTime;
        };
        auto startDownIn = [&]() {
            applyClipFrame(registry, id, *downClip, 0);
            dirAnim.phase     = DirectionalAnimationComponent::Phase::DownIn;
            dirAnim.phaseTime = downClip->frameTime;
        };
        auto startIdle = [&]() {
            applyClipFrame(registry, id, *idleClip, 0);
            dirAnim.phase     = DirectionalAnimationComponent::Phase::Idle;
            dirAnim.phaseTime = 0.0F;
        };

        switch (dirAnim.phase) {
            case DirectionalAnimationComponent::Phase::Idle:
                if (upIntent) {
                    startUpIn();
                } else if (downIntent) {
                    startDownIn();
                } else if (dirAnim.phaseTime < 0.0F) {
                    applyClipFrame(registry, id, *idleClip, 0);
                    dirAnim.phaseTime = 0.0F;
                }
                break;
            case DirectionalAnimationComponent::Phase::UpIn:
                if (downIntent) {
                    startDownIn();
                    break;
                }
                dirAnim.phaseTime -= deltaTime;
                if (dirAnim.phaseTime <= 0.0F) {
                    applyClipFrame(registry, id, *upClip, static_cast<std::uint32_t>(upClip->frames.size() - 1));
                    dirAnim.phase = DirectionalAnimationComponent::Phase::UpHold;
                }
                break;
            case DirectionalAnimationComponent::Phase::UpHold:
                if (downIntent) {
                    startDownIn();
                } else if (!upIntent) {
                    applyClipFrame(registry, id, *upClip, static_cast<std::uint32_t>(upClip->frames.size() - 1));
                    dirAnim.phase     = DirectionalAnimationComponent::Phase::UpOut;
                    dirAnim.phaseTime = upClip->frameTime;
                }
                break;
            case DirectionalAnimationComponent::Phase::UpOut:
                dirAnim.phaseTime -= deltaTime;
                if (dirAnim.phaseTime <= 0.0F) {
                    startIdle();
                }
                break;
            case DirectionalAnimationComponent::Phase::DownIn:
                if (upIntent) {
                    startUpIn();
                    break;
                }
                dirAnim.phaseTime -= deltaTime;
                if (dirAnim.phaseTime <= 0.0F) {
                    applyClipFrame(registry, id, *downClip, static_cast<std::uint32_t>(downClip->frames.size() - 1));
                    dirAnim.phase = DirectionalAnimationComponent::Phase::DownHold;
                }
                break;
            case DirectionalAnimationComponent::Phase::DownHold:
                if (upIntent) {
                    startUpIn();
                } else if (!downIntent) {
                    applyClipFrame(registry, id, *downClip, static_cast<std::uint32_t>(downClip->frames.size() - 1));
                    dirAnim.phase     = DirectionalAnimationComponent::Phase::DownOut;
                    dirAnim.phaseTime = downClip->frameTime;
                }
                break;
            case DirectionalAnimationComponent::Phase::DownOut:
                dirAnim.phaseTime -= deltaTime;
                if (dirAnim.phaseTime <= 0.0F) {
                    startIdle();
                }
                break;
        }
    }
}
