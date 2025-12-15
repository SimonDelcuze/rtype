#pragma once

#include "animation/AnimationLabels.hpp"
#include "animation/AnimationRegistry.hpp"
#include "systems/ISystem.hpp"

class DirectionalAnimationSystem : public ISystem
{
  public:
    DirectionalAnimationSystem(AnimationRegistry& animations, AnimationLabels& labels);
    void update(Registry& registry, float deltaTime) override;

  private:
    void applyClipFrame(Registry& registry, EntityId id, const AnimationClip& clip, std::uint32_t frameIndex);
    AnimationRegistry* animations_{nullptr};
    AnimationLabels* labels_{nullptr};
};
