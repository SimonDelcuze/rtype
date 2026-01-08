#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

using EntityId = std::uint32_t;

struct ClientEntityState
{
    float posX          = 0.0F;
    float posY          = 0.0F;
    float velX          = 0.0F;
    float velY          = 0.0F;
    std::int16_t health = 0;
    bool valid          = false;
};

struct ClientStateSnapshot
{
    std::uint64_t tick = 0;
    std::unordered_map<EntityId, ClientEntityState> entities;
    std::uint32_t checksum = 0;
    bool valid             = false;

    ClientStateSnapshot() = default;

    ClientStateSnapshot(const ClientStateSnapshot&)            = delete;
    ClientStateSnapshot& operator=(const ClientStateSnapshot&) = delete;

    ClientStateSnapshot(ClientStateSnapshot&&) noexcept            = default;
    ClientStateSnapshot& operator=(ClientStateSnapshot&&) noexcept = default;
};

class ClientStateHistory
{
  public:
    static constexpr std::size_t HISTORY_SIZE = 60;

    ClientStateHistory() : head_(0), count_(0)
    {
        snapshots_.resize(HISTORY_SIZE);
    }

    void addSnapshot(std::uint64_t tick, const std::unordered_map<EntityId, ClientEntityState>& entities,
                     std::uint32_t checksum)
    {
        ClientStateSnapshot& snapshot = snapshots_[head_];
        snapshot.tick                 = tick;
        snapshot.entities             = entities;
        snapshot.checksum             = checksum;
        snapshot.valid                = true;

        head_ = (head_ + 1) % HISTORY_SIZE;
        if (count_ < HISTORY_SIZE) {
            count_++;
        }
    }

    std::optional<std::reference_wrapper<const ClientStateSnapshot>> getSnapshot(std::uint64_t tick) const
    {
        for (std::size_t i = 0; i < count_; i++) {
            std::size_t idx = (head_ + HISTORY_SIZE - 1 - i) % HISTORY_SIZE;
            if (snapshots_[idx].valid && snapshots_[idx].tick == tick) {
                return std::cref(snapshots_[idx]);
            }
        }
        return std::nullopt;
    }

    std::optional<std::reference_wrapper<const ClientStateSnapshot>> getLatest() const
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
    std::vector<ClientStateSnapshot> snapshots_;
    std::size_t head_;
    std::size_t count_;
};
