#pragma once

#include "replication/EntityStateCache.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

using EntityId = std::uint32_t;

struct StateSnapshot
{
    std::uint64_t tick = 0;
    std::unordered_map<EntityId, CachedEntityState> entities;
    std::uint32_t checksum = 0;
    bool valid             = false;

    StateSnapshot() = default;

    StateSnapshot(const StateSnapshot&)            = delete;
    StateSnapshot& operator=(const StateSnapshot&) = delete;

    StateSnapshot(StateSnapshot&&) noexcept            = default;
    StateSnapshot& operator=(StateSnapshot&&) noexcept = default;
};

class StateHistory
{
  public:
    static constexpr std::size_t HISTORY_SIZE = 60;

    StateHistory() : head_(0), count_(0)
    {
        snapshots_.resize(HISTORY_SIZE);
    }

    void addSnapshot(std::uint64_t tick, const std::unordered_map<EntityId, CachedEntityState>& entities,
                     std::uint32_t checksum)
    {
        StateSnapshot& snapshot = snapshots_[head_];
        snapshot.tick           = tick;
        snapshot.entities       = entities;
        snapshot.checksum       = checksum;
        snapshot.valid          = true;

        head_ = (head_ + 1) % HISTORY_SIZE;
        if (count_ < HISTORY_SIZE) {
            count_++;
        }
    }

    std::optional<std::reference_wrapper<const StateSnapshot>> getSnapshot(std::uint64_t tick) const
    {
        for (std::size_t i = 0; i < count_; i++) {
            std::size_t idx = (head_ + HISTORY_SIZE - 1 - i) % HISTORY_SIZE;
            if (snapshots_[idx].valid && snapshots_[idx].tick == tick) {
                return std::cref(snapshots_[idx]);
            }
        }
        return std::nullopt;
    }

    std::optional<std::reference_wrapper<const StateSnapshot>> getLatest() const
    {
        if (count_ == 0) {
            return std::nullopt;
        }

        std::size_t latestIdx = (head_ + HISTORY_SIZE - 1) % HISTORY_SIZE;
        if (snapshots_[latestIdx].valid) {
            return std::cref(snapshots_[latestIdx]);
        }
        return std::nullopt;
    }

    std::optional<std::reference_wrapper<const StateSnapshot>> getOldest() const
    {
        if (count_ == 0) {
            return std::nullopt;
        }

        std::size_t oldestIdx = (count_ < HISTORY_SIZE) ? 0 : head_;
        if (snapshots_[oldestIdx].valid) {
            return std::cref(snapshots_[oldestIdx]);
        }
        return std::nullopt;
    }

    std::optional<std::pair<std::uint64_t, std::uint64_t>> getTickRange() const
    {
        auto oldest = getOldest();
        auto latest = getLatest();

        if (oldest && latest) {
            return std::make_pair(oldest->get().tick, latest->get().tick);
        }
        return std::nullopt;
    }

    bool hasSnapshot(std::uint64_t tick) const
    {
        return getSnapshot(tick).has_value();
    }

    void clear()
    {
        for (auto& snapshot : snapshots_) {
            snapshot.valid = false;
            snapshot.entities.clear();
        }
        head_  = 0;
        count_ = 0;
    }

    std::size_t size() const
    {
        return count_;
    }

    bool empty() const
    {
        return count_ == 0;
    }

  private:
    std::vector<StateSnapshot> snapshots_;
    std::size_t head_;
    std::size_t count_;
};
