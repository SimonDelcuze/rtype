#include "game/GameInstanceManager.hpp"

#include "Logger.hpp"

#include <algorithm>

GameInstanceManager::GameInstanceManager(std::uint16_t basePort, std::uint32_t maxInstances,
                                         std::atomic<bool>& runningFlag, bool enableTui, bool enableAdmin)
    : basePort_(basePort), maxInstances_(maxInstances), running_(&runningFlag), enableTui_(enableTui),
      enableAdmin_(enableAdmin)
{
    Logger::instance().info("[InstanceManager] Initialized with max instances: " + std::to_string(maxInstances));
}

std::optional<std::uint32_t> GameInstanceManager::createInstance()
{
    std::lock_guard<std::mutex> lock(instancesMutex_);

    if (instances_.size() >= maxInstances_) {
        Logger::instance().warn("[InstanceManager] Cannot create instance: max instances (" +
                                std::to_string(maxInstances_) + ") reached");
        return std::nullopt;
    }

    std::uint32_t roomId = nextRoomId_++;

    std::uint16_t instancePort = basePort_ + static_cast<std::uint16_t>(roomId);

    auto instance = std::make_unique<GameInstance>(roomId, instancePort, *running_, enableTui_, enableAdmin_);

    if (!instance->start()) {
        Logger::instance().error("[InstanceManager] Failed to start instance " + std::to_string(roomId));
        return std::nullopt;
    }

    Logger::instance().info("[InstanceManager] Created instance " + std::to_string(roomId) + " on port " +
                            std::to_string(instancePort));

    instances_[roomId] = std::move(instance);

    return roomId;
}

void GameInstanceManager::destroyInstance(std::uint32_t roomId)
{
    std::lock_guard<std::mutex> lock(instancesMutex_);

    auto it = instances_.find(roomId);
    if (it == instances_.end()) {
        Logger::instance().warn("[InstanceManager] Attempt to destroy non-existent instance " +
                                std::to_string(roomId));
        return;
    }

    it->second->stop();
    instances_.erase(it);

    Logger::instance().info("[InstanceManager] Destroyed instance " + std::to_string(roomId));
}

GameInstance* GameInstanceManager::getInstance(std::uint32_t roomId)
{
    std::lock_guard<std::mutex> lock(instancesMutex_);

    auto it = instances_.find(roomId);
    if (it == instances_.end()) {
        return nullptr;
    }

    return it->second.get();
}

bool GameInstanceManager::hasInstance(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(instancesMutex_);
    return instances_.find(roomId) != instances_.end();
}

std::size_t GameInstanceManager::getInstanceCount() const
{
    std::lock_guard<std::mutex> lock(instancesMutex_);
    return instances_.size();
}

std::vector<std::uint32_t> GameInstanceManager::getAllRoomIds() const
{
    std::lock_guard<std::mutex> lock(instancesMutex_);

    std::vector<std::uint32_t> roomIds;
    roomIds.reserve(instances_.size());

    for (const auto& [roomId, _] : instances_) {
        roomIds.push_back(roomId);
    }

    return roomIds;
}

void GameInstanceManager::cleanupEmptyInstances()
{
    std::lock_guard<std::mutex> lock(instancesMutex_);

    std::vector<std::uint32_t> toDestroy;

    for (const auto& [roomId, instance] : instances_) {
        if (instance->isEmpty()) {
            toDestroy.push_back(roomId);
        }
    }

    for (std::uint32_t roomId : toDestroy) {
        auto it = instances_.find(roomId);
        if (it != instances_.end()) {
            it->second->stop();
            instances_.erase(it);
            Logger::instance().info("[InstanceManager] Cleaned up empty instance " + std::to_string(roomId));
        }
    }
}
