#include "console/commands/SystemCommands.hpp"

#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"
#include "game/GameInstanceManager.hpp"

bool SystemCommands::handleCommand(ServerConsole* console, const std::string& cmd,
                                   const std::function<void()>& shutdownCallback,
                                   const std::function<void(const std::string&)>& broadcastCallback)
{
    std::string lowerCmd = CommandUtils::toLower(cmd);

    if (lowerCmd == "help") {
        handleHelp(console);
        return true;
    }
    if (lowerCmd == "stats") {
        handleStats(console);
        return true;
    }
    if (lowerCmd == "ping") {
        handlePing(console);
        return true;
    }
    if (lowerCmd == "quit" || lowerCmd == "q" || lowerCmd == "stop" || lowerCmd == "exit" || lowerCmd == "leave") {
        handleQuit(console, shutdownCallback);
        return true;
    }

    if (lowerCmd.find("broadcast ") == 0) {
        std::string msg = cmd.substr(10);
        if (broadcastCallback) {
            broadcastCallback(msg);
            console->addAdminLog("[Admin] Broadcast sent: " + msg);
        } else {
            console->addAdminLog("[Error] Broadcast callback not set!");
        }
        return true;
    }

    return false;
}

void SystemCommands::handleHelp(ServerConsole* console)
{
    console->addAdminLog("Available commands:");
    console->addAdminLog("  rooms       - List all active rooms");
    console->addAdminLog("  kill <id>   - Force-stop a room");
    console->addAdminLog("  logs <id>   - Filter logs to room");
    console->addAdminLog("  logs all    - Show all logs");
    console->addAdminLog("  tags list   - List enabled/available tags");
    console->addAdminLog("  tags add <tag>    - Enable a log tag");
    console->addAdminLog("  tags remove <tag> - Disable a log tag");
    console->addAdminLog("  stats       - Show detailed stats");
    console->addAdminLog("  broadcast <msg> - Send a message to all clients");
    console->addAdminLog("  quit        - Shutdown the server (q stop exit)");
    console->addAdminLog("  help        - Show this help");
}

void SystemCommands::handleStats(ServerConsole* console)
{
    const auto& currentStats = console->getCurrentStats();
    console->addAdminLog("[Stats] Total bandwidth:");
    console->addAdminLog("  IN:  " + CommandUtils::formatBytes(currentStats.bytesIn) + " (" +
                         std::to_string(currentStats.packetsIn) + " packets)");
    console->addAdminLog("  OUT: " + CommandUtils::formatBytes(currentStats.bytesOut) + " (" +
                         std::to_string(currentStats.packetsOut) + " packets)");
    console->addAdminLog("  LOSS: " + std::to_string(currentStats.packetsLost) + " packets");
}

void SystemCommands::handleQuit(ServerConsole* console, const std::function<void()>& shutdownCallback)
{
    console->addAdminLog("[Admin] Shutting down server...");
    if (shutdownCallback) {
        shutdownCallback();
    } else {
        console->addAdminLog("[Error] Shutdown callback not set!");
    }
}

void SystemCommands::handlePing(ServerConsole* console)
{
    console->addAdminLog("[Pong] Server is alive!");
}
