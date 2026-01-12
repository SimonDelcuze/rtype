#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"
#include "network/InputParser.hpp"
#include "simulation/PlayerCommand.hpp"

class PlayerInputSystem
{
  public:
    PlayerInputSystem(float speed = 1.0F, float missileSpeed = 5.0F, float missileLifetime = 2.0F,
                      std::int32_t missileDamage = 1);
    void setTuning(float speed, float missileSpeed, float missileLifetime, std::int32_t missileDamage)
    {
        speed_           = speed;
        missileSpeed_    = missileSpeed;
        missileLifetime_ = missileLifetime;
        missileDamage_   = missileDamage;
    }
    void update(Registry& registry, const std::vector<PlayerCommand>& commands) const;

  private:
    int chargeLevelFromFlags(std::uint16_t flags) const;

    float speed_;
    float missileSpeed_;
    float missileLifetime_;
    std::int32_t missileDamage_;
};
