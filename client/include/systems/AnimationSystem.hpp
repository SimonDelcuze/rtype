#pragma once

#include "components/AnimationComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/ISystem.hpp"

class AnimationSystem : public ISystem
{
  public:
    void update(Registry& registry, float deltaTime) override;

  private:
    void advanceFrame(AnimationComponent& anim);
};
