#pragma once

#include "rollback/StateChecksum.hpp"
#include "rollback/StateHistory.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

class Registry;
using EntityId = std::uint32_t;

class RollbackManager
{
  public:
    RollbackManager();

    std::uint32_t captureState(std::uint64_t tick, const Registry& registry);

    std::optional<std::reference_wrapper<const StateSnapshot>> getSnapshot(std::uint64_t tick) const;

    bool canRollbackTo(std::uint64_t tick) const;

    std::optional<std::uint32_t> getChecksum(std::uint64_t tick) const;

    std::optional<std::pair<std::uint64_t, std::uint64_t>> getTickRange() const;

    bool rollbackTo(std::uint64_t tick, Registry& registry);

    void clear();

    std::size_t getHistorySize() const;

  private:
    std::unordered_map<EntityId, CachedEntityState> extractEntityStates(const Registry& registry)
        const;

    void restoreEntityStates(Registry& registry,
                             const std::unordered_map<EntityId, CachedEntityState>& states) const;

    StateHistory stateHistory_;
    mutable std::mutex historyMutex_;
};
