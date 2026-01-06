#include "lobby/LobbyManager.hpp"

#include "Logger.hpp"

LobbyManager::LobbyManager()
{
    Logger::instance().info("[LobbyManager] Initialized");
}

void LobbyManager::addRoom(std::uint32_t roomId, std::uint16_t port, std::size_t maxPlayers)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    RoomInfo info;
    info.roomId      = roomId;
    info.playerCount = 0;
    info.maxPlayers  = maxPlayers;
    info.state       = RoomState::Waiting;
    info.port        = port;

    rooms_[roomId] = info;

    Logger::instance().info("[LobbyManager] Added room " + std::to_string(roomId) + " on port " + std::to_string(port));
}

void LobbyManager::removeRoom(std::uint32_t roomId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    rooms_.erase(it);

    Logger::instance().info("[LobbyManager] Removed room " + std::to_string(roomId));
}

void LobbyManager::updateRoomState(std::uint32_t roomId, RoomState state)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    it->second.state = state;
}

void LobbyManager::updateRoomPlayerCount(std::uint32_t roomId, std::size_t playerCount)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    it->second.playerCount = playerCount;
}

std::optional<RoomInfo> LobbyManager::getRoomInfo(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<RoomInfo> LobbyManager::listRooms() const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    std::vector<RoomInfo> result;
    result.reserve(rooms_.size());

    for (const auto& [_, info] : rooms_) {
        result.push_back(info);
    }

    return result;
}

bool LobbyManager::roomExists(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    return rooms_.find(roomId) != rooms_.end();
}
