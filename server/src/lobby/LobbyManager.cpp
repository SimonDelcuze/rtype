#include "lobby/LobbyManager.hpp"

#include "Logger.hpp"
#include "lobby/PasswordUtils.hpp"

#include <algorithm>

LobbyManager::LobbyManager()
{
    Logger::instance().info("[LobbyManager] Initialized");
}

void LobbyManager::addRoom(std::uint32_t roomId, std::uint16_t port, std::size_t maxPlayers, RoomType roomType)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    RoomInfo info;
    info.roomId      = roomId;
    info.roomType    = roomType;
    info.playerCount = 0;
    info.maxPlayers  = maxPlayers;
    info.state       = RoomState::Waiting;
    info.port        = port;
    info.config      = RoomConfig::preset(RoomDifficulty::Hell);
    info.countdown   = 0;

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
    roomPlayers_.erase(roomId);
    playerReadyStatus_.erase(roomId);
    playerSpectatorStatus_.erase(roomId);
    rankedCountdowns_.erase(roomId);
    playerNames_.erase(roomId);
}

void LobbyManager::updateRoomState(std::uint32_t roomId, RoomState state)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    RoomState oldState = it->second.state;
    it->second.state   = state;

    if (oldState == RoomState::Playing && state == RoomState::Waiting) {
        Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) +
                                " returned to Waiting state, clearing player list");
        roomPlayers_.erase(roomId);
    }
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
        if (info.state != RoomState::Finished) {
            result.push_back(info);
        }
    }

    return result;
}

bool LobbyManager::roomExists(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    return rooms_.find(roomId) != rooms_.end();
}

bool LobbyManager::hasRoomOfType(RoomType type) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    for (const auto& [_, info] : rooms_) {
        if (info.roomType == type) {
            return true;
        }
    }
    return false;
}

bool LobbyManager::hasWaitingRoomOfType(RoomType type) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    for (const auto& [_, info] : rooms_) {
        if (info.roomType == type && (info.state == RoomState::Waiting || info.state == RoomState::Countdown)) {
            return true;
        }
    }
    return false;
}

void LobbyManager::setRoomOwner(std::uint32_t roomId, std::uint32_t ownerId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    it->second.ownerId = ownerId;
    Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) + " owner set to " +
                            std::to_string(ownerId));
}

void LobbyManager::addRoomAdmin(std::uint32_t roomId, std::uint32_t playerId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    auto& admins = it->second.adminIds;
    if (std::find(admins.begin(), admins.end(), playerId) == admins.end()) {
        admins.push_back(playerId);
        Logger::instance().info("[LobbyManager] Added admin " + std::to_string(playerId) + " to room " +
                                std::to_string(roomId));
    }
}

void LobbyManager::removeRoomAdmin(std::uint32_t roomId, std::uint32_t playerId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    auto& admins = it->second.adminIds;
    admins.erase(std::remove(admins.begin(), admins.end(), playerId), admins.end());
    Logger::instance().info("[LobbyManager] Removed admin " + std::to_string(playerId) + " from room " +
                            std::to_string(roomId));
}

void LobbyManager::addBannedPlayer(std::uint32_t roomId, std::uint32_t playerId, const std::string& ipAddress)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    auto& bannedIds = it->second.bannedPlayerIds;
    if (std::find(bannedIds.begin(), bannedIds.end(), playerId) == bannedIds.end()) {
        bannedIds.push_back(playerId);
    }

    auto& bannedIPs = it->second.bannedIPs;
    if (std::find(bannedIPs.begin(), bannedIPs.end(), ipAddress) == bannedIPs.end()) {
        bannedIPs.push_back(ipAddress);
    }

    Logger::instance().info("[LobbyManager] Banned player " + std::to_string(playerId) + " (" + ipAddress +
                            ") from room " + std::to_string(roomId));
}

void LobbyManager::removeBannedPlayer(std::uint32_t roomId, std::uint32_t playerId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    auto& bannedIds = it->second.bannedPlayerIds;
    bannedIds.erase(std::remove(bannedIds.begin(), bannedIds.end(), playerId), bannedIds.end());
    Logger::instance().info("[LobbyManager] Unbanned player " + std::to_string(playerId) + " from room " +
                            std::to_string(roomId));
}

bool LobbyManager::isPlayerBanned(std::uint32_t roomId, std::uint32_t playerId, const std::string& ipAddress) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return false;
    }

    const auto& bannedIds = it->second.bannedPlayerIds;
    if (playerId != 0 && std::find(bannedIds.begin(), bannedIds.end(), playerId) != bannedIds.end()) {
        return true;
    }

    const auto& bannedIPs = it->second.bannedIPs;
    if (!ipAddress.empty() && std::find(bannedIPs.begin(), bannedIPs.end(), ipAddress) != bannedIPs.end()) {
        return true;
    }

    return false;
}

void LobbyManager::setRoomName(std::uint32_t roomId, const std::string& name)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    it->second.roomName = name;
    Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) + " name set to: " + name);
}

void LobbyManager::setRoomPassword(std::uint32_t roomId, const std::string& passwordHash)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    if (passwordHash.empty()) {
        it->second.passwordProtected = false;
        it->second.passwordHash.clear();
        Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) + " password removed");
    } else {
        it->second.passwordProtected = true;
        it->second.passwordHash      = passwordHash;
        Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) + " password set");
    }
}

void LobbyManager::setRoomVisibility(std::uint32_t roomId, RoomVisibility visibility)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    it->second.visibility = visibility;
    Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) + " visibility set to " +
                            std::to_string(static_cast<int>(visibility)));
}

void LobbyManager::setRoomConfig(std::uint32_t roomId, const RoomConfig& config)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }

    RoomConfig cfg = config;
    if (cfg.mode == RoomDifficulty::Custom) {
        cfg.clampCustom();
    }
    it->second.config = cfg;
    Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) +
                            " config set (mode=" + std::to_string(static_cast<int>(cfg.mode)) + ")");
}

std::string LobbyManager::generateAndSetInviteCode(std::uint32_t roomId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return "";
    }

    std::string code      = PasswordUtils::generateInviteCode();
    it->second.inviteCode = code;
    Logger::instance().info("[LobbyManager] Room " + std::to_string(roomId) + " invite code set to: " + code);
    return code;
}

bool LobbyManager::verifyRoomPassword(std::uint32_t roomId, const std::string& passwordHash) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return false;
    }

    if (!it->second.passwordProtected) {
        return true;
    }

    return it->second.passwordHash == passwordHash;
}

void LobbyManager::addPlayerToRoom(std::uint32_t roomId, std::uint32_t playerId, const std::string& displayName)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto& players = roomPlayers_[roomId];
    if (std::find(players.begin(), players.end(), playerId) == players.end()) {
        players.push_back(playerId);
        playerReadyStatus_[roomId][playerId] = false;
        if (!displayName.empty()) {
            playerNames_[roomId][playerId] = displayName;
        }

        auto roomIt = rooms_.find(roomId);
        if (roomIt != rooms_.end() && roomIt->second.roomType == RoomType::Ranked) {
            if (rankedCountdowns_.find(roomId) == rankedCountdowns_.end()) {
                rankedCountdowns_[roomId] = 60.0F;
                Logger::instance().info("[LobbyManager] Started 60s auto-start timer for Room " +
                                        std::to_string(roomId));
            }
        }
        Logger::instance().info("[LobbyManager] Player " + std::to_string(playerId) + " added to room " +
                                std::to_string(roomId) + " (now " + std::to_string(players.size()) + " players)");
    }
}

void LobbyManager::removePlayerFromRoom(std::uint32_t roomId, std::uint32_t playerId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = roomPlayers_.find(roomId);
    if (it != roomPlayers_.end()) {
        auto& players = it->second;
        players.erase(std::remove(players.begin(), players.end(), playerId), players.end());
        Logger::instance().info("[LobbyManager] Player " + std::to_string(playerId) + " removed from room " +
                                std::to_string(roomId) + " (now " + std::to_string(players.size()) + " players)");
    }

    if (auto nameIt = playerNames_.find(roomId); nameIt != playerNames_.end()) {
        nameIt->second.erase(playerId);
    }
}

std::vector<std::uint32_t> LobbyManager::getRoomPlayers(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto it = roomPlayers_.find(roomId);
    if (it != roomPlayers_.end()) {
        return it->second;
    }
    return {};
}

std::optional<std::string> LobbyManager::getPlayerName(std::uint32_t roomId, std::uint32_t playerId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    auto roomIt = playerNames_.find(roomId);
    if (roomIt == playerNames_.end()) {
        return std::nullopt;
    }

    auto it = roomIt->second.find(playerId);
    if (it == roomIt->second.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool LobbyManager::handlePlayerDisconnect(std::uint32_t roomId, std::uint32_t playerId)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);

    auto roomIt = rooms_.find(roomId);
    if (roomIt == rooms_.end()) {
        return false;
    }

    bool wasOwner = (roomIt->second.ownerId == playerId);

    auto playersIt = roomPlayers_.find(roomId);
    if (playersIt != roomPlayers_.end()) {
        auto& players = playersIt->second;
        players.erase(std::remove(players.begin(), players.end(), playerId), players.end());
        Logger::instance().info("[LobbyManager] Player " + std::to_string(playerId) + " disconnected from room " +
                                std::to_string(roomId) + " (owner=" + (wasOwner ? "true" : "false") + ", " +
                                std::to_string(players.size()) + " players remaining)");
    }

    if (wasOwner) {
        Logger::instance().info("[LobbyManager] Room owner left, deleting room " + std::to_string(roomId));
        rooms_.erase(roomIt);
        roomPlayers_.erase(roomId);
        playerReadyStatus_.erase(roomId);
        playerSpectatorStatus_.erase(roomId);
        rankedCountdowns_.erase(roomId);
        playerNames_.erase(roomId);
        return true;
    }

    playerReadyStatus_[roomId].erase(playerId);
    playerSpectatorStatus_[roomId].erase(playerId);
    playerNames_[roomId].erase(playerId);

    return false;
}

void LobbyManager::setPlayerReady(std::uint32_t roomId, std::uint32_t playerId, bool ready)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    playerReadyStatus_[roomId][playerId] = ready;
    Logger::instance().info("[LobbyManager] Player " + std::to_string(playerId) + " in room " + std::to_string(roomId) +
                            " is now " + (ready ? "READY" : "NOT READY"));
}

bool LobbyManager::isPlayerReady(std::uint32_t roomId, std::uint32_t playerId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    auto it = playerReadyStatus_.find(roomId);
    if (it != playerReadyStatus_.end()) {
        auto pIt = it->second.find(playerId);
        if (pIt != it->second.end()) {
            return pIt->second;
        }
    }
    return false;
}

bool LobbyManager::isRoomAllReady(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    auto pIt = roomPlayers_.find(roomId);
    if (pIt == roomPlayers_.end() || pIt->second.empty())
        return false;

    auto rIt = playerReadyStatus_.find(roomId);
    if (rIt == playerReadyStatus_.end())
        return false;

    for (std::uint32_t playerId : pIt->second) {
        auto statusIt = rIt->second.find(playerId);
        if (statusIt == rIt->second.end() || !statusIt->second) {
            return false;
        }
    }
    return true;
}

void LobbyManager::updateRankedCountdowns(float dt)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    for (auto it = rankedCountdowns_.begin(); it != rankedCountdowns_.end();) {
        it->second -= dt;
        auto roomIt = rooms_.find(it->first);
        if (roomIt != rooms_.end()) {
            roomIt->second.countdown = static_cast<std::uint8_t>(std::max(0.0F, it->second));
        }
        if (it->second <= 0.0F) {
            it = rankedCountdowns_.erase(it);
        } else {
            ++it;
        }
    }
}

std::uint8_t LobbyManager::getRoomCountdown(std::uint32_t roomId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    auto it = rankedCountdowns_.find(roomId);
    if (it != rankedCountdowns_.end()) {
        return static_cast<std::uint8_t>(std::max(0.0F, it->second));
    }
    return 0;
}

void LobbyManager::setPlayerSpectator(std::uint32_t roomId, std::uint32_t playerId, bool spectator)
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    playerSpectatorStatus_[roomId][playerId] = spectator;
}

bool LobbyManager::isPlayerSpectator(std::uint32_t roomId, std::uint32_t playerId) const
{
    std::lock_guard<std::mutex> lock(roomsMutex_);
    auto roomIt = playerSpectatorStatus_.find(roomId);
    if (roomIt != playerSpectatorStatus_.end()) {
        auto playerIt = roomIt->second.find(playerId);
        if (playerIt != roomIt->second.end()) {
            return playerIt->second;
        }
    }
    return false;
}
