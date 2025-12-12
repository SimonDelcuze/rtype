#pragma once

#include "server/ILevel.hpp"

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
    float wave1Offset_              = 1.0F;
    float wave2Offset_              = 9.0F;
    float wave3Offset_              = 17.0F;
    std::size_t wave1ShooterModulo_ = 4;
    std::size_t wave2ShooterModulo_ = 4;
    std::size_t wave3ShooterModulo_ = 4;

    std::vector<MovementComponent> makePatterns() const;
    std::vector<SpawnEvent> buildTimeline(const HitboxComponent& hitbox, const EnemyShootingComponent& shooting) const;
};
