#pragma once

#include "systems/ISystem.hpp"

class Registry;

class InvincibilitySystem : public ISystem
{
  public:
    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}
};
