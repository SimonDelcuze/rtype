#pragma once

#include "levels/ILevel.hpp"

#include <array>
#include <vector>

class Level1 : public ILevel
{
  public:
    int id() const override
    {
        return 1;
    }
    LevelScript buildScript() const override;

  private:
    std::vector<MovementComponent> makePatterns() const;
    std::vector<SpawnEvent> buildTimeline(const HitboxComponent& hitbox, const EnemyShootingComponent& shooting) const;
    std::vector<ObstacleSpawn> buildObstacles(const HitboxComponent& topHitbox, const HitboxComponent& midHitbox,
                                              const HitboxComponent& bottomHitbox,
                                              const std::vector<std::array<float, 2>>& topHull,
                                              const std::vector<std::array<float, 2>>& midHull,
                                              const std::vector<std::array<float, 2>>& bottomHull) const;
};
