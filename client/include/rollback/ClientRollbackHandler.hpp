#pragma once

#include "rollback/ClientStateHistory.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>

class Registry;

class ClientRollbackHandler
{
  public:
    ClientRollbackHandler();

    std::uint32_t captureState(std::uint64_t tick, const Registry& registry);

    bool handleRollbackRequest(std::uint64_t rollbackToTick, std::uint64_t currentTick, Registry& registry);

    std::optional<std::uint32_t> getChecksum(std::uint64_t tick) const;

    bool hasSnapshot(std::uint64_t tick) const;

    void clear();

    std::size_t getHistorySize() const;

    void setRollbackCallback(std::function<void(std::uint64_t, std::uint64_t)> callback);

  private:
    std::unordered_map<EntityId, ClientEntityState> extractEntityStates(const Registry& registry) const;

    void restoreEntityStates(Registry& registry, const std::unordered_map<EntityId, ClientEntityState>& states) const;

    std::uint32_t computeChecksum(const std::unordered_map<EntityId, ClientEntityState>& states) const;

    ClientStateHistory stateHistory_;
    mutable std::mutex historyMutex_;
    std::function<void(std::uint64_t, std::uint64_t)> rollbackCallback_;
    std::mutex callbackMutex_;
};
