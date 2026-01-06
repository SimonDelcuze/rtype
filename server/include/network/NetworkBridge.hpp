#pragma once

#include "network/SendThread.hpp"
#include "simulation/GameEvent.hpp"

#include <cstdint>
#include <unordered_set>
#include <vector>

using EntityId = std::uint32_t;

class NetworkBridge
{
  public:
    explicit NetworkBridge(SendThread& sendThread);

    void processEvents(const std::vector<GameEvent>& events);

    void clear();

  private:
    SendThread& sendThread_;
    std::unordered_set<EntityId> knownEntities_;
};
