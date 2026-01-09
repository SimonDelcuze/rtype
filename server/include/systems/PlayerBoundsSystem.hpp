#pragma once

#include "ecs/Registry.hpp"
#include "levels/LevelData.hpp"

class PlayerBoundsSystem
{
  public:
    void update(Registry& registry, const std::optional<CameraBounds>& bounds);
    void reset();

  private:
    std::optional<CameraBounds> active_;
    std::optional<CameraBounds> defaults_;

    CameraBounds fallbackBounds() const;
    std::optional<CameraBounds> readDefaults(Registry& registry) const;
    void applyBounds(Registry& registry, const CameraBounds& bounds) const;
};
