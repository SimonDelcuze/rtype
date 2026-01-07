#include "console/commands/RoomCommands.hpp"

#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"
#include "game/GameInstanceManager.hpp"

void RoomCommands::handleCommand(ServerConsole* console, GameInstanceManager* instanceManager, const std::string& cmd)
{
    std::string lowerCmd = CommandUtils::toLower(cmd);

    if (lowerCmd == "rooms") {
        handleList(console, instanceManager);
    } else if (CommandUtils::startsWithIgnoreCase(cmd, "kill ")) {
        handleKill(console, instanceManager, cmd.substr(5));
    }
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

void RoomCommands::handleKill(ServerConsole* console, GameInstanceManager* instanceManager, const std::string& idArg)
{
    try {
        std::uint32_t roomId = std::stoul(idArg);
        if (instanceManager != nullptr && instanceManager->hasInstance(roomId)) {
            instanceManager->destroyInstance(roomId);
            console->addAdminLog("[Admin] Room " + std::to_string(roomId) + " destroyed");
        } else {
            console->addAdminLog("[Error] Room " + std::to_string(roomId) + " not found");
        }
    } catch (...) {
        console->addAdminLog("[Error] Invalid room ID: " + idArg);
    }
}
