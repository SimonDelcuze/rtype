#pragma once

#include "components/AnimationComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "ecs/Registry.hpp"

class AnimationSystem
{
  public:
    void update(Registry& registry, float deltaTime);

  private:
    void advanceFrame(AnimationComponent& anim);
};
