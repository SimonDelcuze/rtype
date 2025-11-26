#pragma once

#include "components/Components.hpp"
#include "events/DestroyEvent.hpp"

#include <events/EventBus.hpp>
#include <vector>

class DestructionSystem
{
  public:
    explicit DestructionSystem(EventBus& bus);
    void update(Registry& registry, const std::vector<EntityId>& toDestroy);

  private:
    EventBus& bus_;
};
