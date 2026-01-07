#include "console/commands/RoomCommands.hpp"

#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"
#include "game/GameInstanceManager.hpp"
#include "lobby/LobbyManager.hpp"

#include <sstream>
#include <vector>

bool RoomCommands::handleCommand(ServerConsole* console, GameInstanceManager* instanceManager,
                                 LobbyManager* lobbyManager, const std::string& cmd)
{
    std::string lowerCmd = CommandUtils::toLower(cmd);

    if (lowerCmd == "rooms") {
        handleList(console, instanceManager);
        return true;
    }
    if (CommandUtils::startsWithIgnoreCase(cmd, "kill ")) {
        handleKill(console, instanceManager, lobbyManager, cmd.substr(5));
        return true;
    }
    if (CommandUtils::startsWithIgnoreCase(cmd, "list ")) {
        handleListPlayers(console, instanceManager, cmd.substr(5));
        return true;
    }
    if (CommandUtils::startsWithIgnoreCase(cmd, "kick ")) {
        handleKickPlayer(console, instanceManager, cmd.substr(5));
        return true;
    }
    return false;
}

void RoomCommands::handleList(ServerConsole* console, GameInstanceManager* instanceManager)
{
    if (instanceManager == nullptr) {
        console->addAdminLog("[Error] Instance manager not available");
        return;
    }
    auto roomIds = instanceManager->getAllRoomIds();
    if (roomIds.empty()) {
        console->addAdminLog("[Rooms] No active rooms");
        return;
    }
    console->addAdminLog("[Rooms] Active rooms:");
    for (auto roomId : roomIds) {
        auto* instance = instanceManager->getInstance(roomId);
        if (instance != nullptr) {
            console->addAdminLog("  Room " + std::to_string(roomId) +
                                 " | Port: " + std::to_string(instance->getPort()) +
                                 " | Players: " + std::to_string(instance->getPlayerCount()) +
                                 (instance->isGameStarted() ? " | PLAYING" : " | WAITING"));
        }
    }
}

void RoomCommands::handleKill(ServerConsole* console, GameInstanceManager* instanceManager, LobbyManager* lobbyManager,
                              const std::string& idArg)
{
    try {
        std::uint32_t roomId = std::stoul(idArg);
        if (instanceManager != nullptr && instanceManager->hasInstance(roomId)) {
            instanceManager->destroyInstance(roomId);
            if (lobbyManager != nullptr) {
                lobbyManager->removeRoom(roomId);
            }
            console->addAdminLog("[Admin] Room " + std::to_string(roomId) + " destroyed");
        } else {
            console->addAdminLog("[Error] Room " + std::to_string(roomId) + " not found");
        }
    } catch (...) {
        console->addAdminLog("[Error] Invalid room ID: " + idArg);
    }
}

void RoomCommands::handleListPlayers(ServerConsole* console, GameInstanceManager* instanceManager,
                                     const std::string& arg)
{
    try {
        std::uint32_t roomId = std::stoul(arg);
        if (instanceManager == nullptr || !instanceManager->hasInstance(roomId)) {
            console->addAdminLog("[Error] Room " + std::to_string(roomId) + " not found");
            return;
        }

        auto* instance = instanceManager->getInstance(roomId);
        auto sessions  = instance->getSessions();

        if (sessions.empty()) {
            console->addAdminLog("[Room " + std::to_string(roomId) + "] No players connected");
            return;
        }

        console->addAdminLog("[Room " + std::to_string(roomId) + "] value: " + std::to_string(sessions.size()) +
                             " players detected");
        for (const auto& session : sessions) {
            std::string status = session.ready ? "READY" : "JOINED";
            if (session.started)
                status = "PLAYING";
            console->addAdminLog("  - ID: " + std::to_string(session.playerId) +
                                 " | Endpoint: " + endpointKey(session.endpoint) + " | Status: " + status);
        }

    } catch (...) {
        console->addAdminLog("[Error] Invalid room ID: " + arg);
    }
}

void RoomCommands::handleKickPlayer(ServerConsole* console, GameInstanceManager* instanceManager,
                                    const std::string& args)
{
    std::vector<std::string> parts;
    std::stringstream ss(args);
    std::string item;
    while (std::getline(ss, item, ' ')) {
        if (!item.empty()) {
            parts.push_back(item);
        }
    }

    if (parts.size() < 2) {
        console->addAdminLog("[Error] Usage: kick <room_id> <player_id>");
        return;
    }

    try {
        std::uint32_t roomId   = std::stoul(parts[0]);
        std::uint32_t playerId = std::stoul(parts[1]);

        if (instanceManager == nullptr || !instanceManager->hasInstance(roomId)) {
            console->addAdminLog("[Error] Room " + std::to_string(roomId) + " not found");
            return;
        }

        auto* instance = instanceManager->getInstance(roomId);
        instance->kickPlayer(playerId);

    } catch (...) {
        console->addAdminLog("[Error] Invalid arguments. Usage: kick <room_id> <player_id>");
    }
}
