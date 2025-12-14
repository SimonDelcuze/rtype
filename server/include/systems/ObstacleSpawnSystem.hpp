#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <vector>

enum class ObstacleAnchor
{
    Absolute,
    Top,
    Bottom
};

struct ObstacleSpawn
{
    float time       = 0.0F;
    float x          = 0.0F;
    float y          = 0.0F;
    ObstacleAnchor anchor = ObstacleAnchor::Absolute;
    float margin     = 0.0F;
    std::int32_t health = 1;
    std::uint16_t typeId = 9;
    float speedX     = -50.0F;
    float speedY     = 0.0F;
    HitboxComponent hitbox;
};

class ObstacleSpawnSystem
{
  public:
    explicit ObstacleSpawnSystem(std::vector<ObstacleSpawn> obstacles, float playfieldHeight = 720.0F);
    void update(Registry& registry, float deltaTime);
    void reset();

  private:
    float resolveY(const ObstacleSpawn& spawn) const;

    std::vector<ObstacleSpawn> obstacles_;
    float elapsed_         = 0.0F;
    std::size_t nextIndex_ = 0;
    float playfieldHeight_ = 720.0F;
};
