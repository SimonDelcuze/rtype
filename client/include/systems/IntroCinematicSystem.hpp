#pragma once

#include "level/LevelState.hpp"
#include "systems/ISystem.hpp"

class IntroCinematicSystem : public ISystem
{
  public:
    explicit IntroCinematicSystem(LevelState& state);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    LevelState* state_;
};
