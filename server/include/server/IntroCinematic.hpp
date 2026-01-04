#pragma once

#include "ecs/Registry.hpp"
#include "level/IntroCinematicConfig.hpp"

#include <map>
#include <unordered_map>

class IntroCinematic
{
  public:
    void start(const std::map<std::uint32_t, EntityId>& players, Registry& registry);
    void reset();
    bool active() const;
    void update(Registry& registry, const std::map<std::uint32_t, EntityId>& players, float deltaTime);

  private:
    struct PlayerStart
    {
        float x = 0.0F;
        float y = 0.0F;
    };

    std::unordered_map<EntityId, PlayerStart> starts_;
    float elapsed_ = 0.0F;
    bool active_   = false;
};
