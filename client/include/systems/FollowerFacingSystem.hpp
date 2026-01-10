#pragma once

#include "animation/AnimationLabels.hpp"
#include "animation/AnimationRegistry.hpp"
#include "components/AnimationComponent.hpp"
#include "components/FollowerFacingComponent.hpp"
#include "components/RenderTypeComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/VelocityComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/ISystem.hpp"

class FollowerFacingSystem : public ISystem
{
  public:
    FollowerFacingSystem(AnimationRegistry& animations, AnimationLabels& labels);
    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}

  private:
    AnimationRegistry* animations_ = nullptr;
    AnimationLabels* labels_       = nullptr;

    void applyClipFrame(Registry& registry, EntityId id, const AnimationClip& clip, std::uint32_t frameIndex) const;
};
