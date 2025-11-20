#pragma once

#include "components/AnimationComponent.hpp"
#include "ecs/Registry.hpp"

#include <functional>

class AnimationSystem
{
  public:
    using SpriteRectCallback =
        std::function<void(EntityId, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)>;

    void update(Registry& registry, float deltaTime);
    void setSpriteRectCallback(SpriteRectCallback callback);

  private:
    void advanceFrame(AnimationComponent& anim);
    void updateSpriteRect(EntityId entity, const AnimationComponent& anim);

    SpriteRectCallback spriteRectCallback_;
};
