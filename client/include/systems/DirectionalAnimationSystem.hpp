#pragma once

#include "animation/AnimationLabels.hpp"
#include "animation/AnimationRegistry.hpp"
#include "components/DirectionalAnimationComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "systems/ISystem.hpp"

class DirectionalAnimationSystem : public ISystem
{
  public:
    DirectionalAnimationSystem(AnimationRegistry& animations, AnimationLabels& labels);
    void update(Registry& registry, float deltaTime) override;

  private:
    void applyClipFrame(Registry& registry, EntityId id, const AnimationClip& clip, std::uint32_t frameIndex);
    struct Clips
    {
        const AnimationClip* idle{nullptr};
        const AnimationClip* up{nullptr};
        const AnimationClip* down{nullptr};
    };
    Clips resolveClips(const DirectionalAnimationComponent& dirAnim) const;
    std::pair<bool, bool> intents(const VelocityComponent& vel, float threshold) const;
    void handlePhase(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                     bool upIntent, bool downIntent, float deltaTime);
    void handleUpIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                    bool downIntent, float deltaTime);
    void handleUpHold(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                      bool upIntent, bool downIntent);
    void handleUpOut(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                     float deltaTime);
    void handleDownIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                      bool upIntent, float deltaTime);
    void handleDownHold(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                        bool downIntent, bool upIntent);
    void handleDownOut(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips,
                       float deltaTime);
    void startIdle(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips);
    void startUpIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips);
    void startDownIn(Registry& registry, EntityId id, DirectionalAnimationComponent& dirAnim, const Clips& clips);
    AnimationRegistry* animations_{nullptr};
    AnimationLabels* labels_{nullptr};
};
