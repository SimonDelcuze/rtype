#pragma once

#include "components/ScoreComponent.hpp"
#include "components/ScoreValueComponent.hpp"
#include "components/TagComponent.hpp"
#include "ecs/Registry.hpp"
#include "events/DamageEvent.hpp"
#include "events/EventBus.hpp"

class ScoreSystem
{
  public:
    ScoreSystem(EventBus& bus, Registry& registry);

  private:
    void onDamage(const DamageEvent& event);

    Registry* registry_ = nullptr;
};
