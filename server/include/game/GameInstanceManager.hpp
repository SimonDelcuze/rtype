#pragma once

#include "game/GameInstance.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

class GameInstanceManager
{
  public:
    GameInstanceManager(std::uint16_t basePort, std::uint32_t maxInstances, std::atomic<bool>& runningFlag);

    std::optional<std::uint32_t> createInstance();

    void destroyInstance(std::uint32_t roomId);

    GameInstance* getInstance(std::uint32_t roomId);

    bool hasInstance(std::uint32_t roomId) const;

    std::size_t getInstanceCount() const;

    std::uint32_t getMaxInstances() const
    {
        return maxInstances_;
    }

    std::vector<std::uint32_t> getAllRoomIds() const;

    void cleanupEmptyInstances();

    void broadcast(const std::string& message);

  private:
    std::uint16_t basePort_;
    std::uint32_t maxInstances_;
    std::uint32_t nextRoomId_{1};
    std::atomic<bool>* running_{nullptr};
    mutable std::mutex instancesMutex_;
    std::map<std::uint32_t, std::unique_ptr<GameInstance>> instances_;
};
