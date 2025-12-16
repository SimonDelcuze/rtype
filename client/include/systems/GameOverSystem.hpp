#pragma once

#include "events/EventBus.hpp"
#include "systems/ISystem.hpp"

class Registry;

class GameOverSystem : public ISystem
{
  public:
    explicit GameOverSystem(EventBus& eventBus);

    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}

  private:
    EventBus& eventBus_;
    bool gameOverTriggered_ = false;
};
