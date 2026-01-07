#include "console/commands/SystemCommands.hpp"

#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"

void SystemCommands::handleCommand(ServerConsole* console, const std::string& cmd,
                                   const std::function<void()>& shutdownCallback)
{
    std::string lowerCmd = CommandUtils::toLower(cmd);

    if (lowerCmd == "help") {
        handleHelp(console);
    } else if (lowerCmd == "stats") {
        handleStats(console);
    } else if (lowerCmd == "ping") {
        handlePing(console);
    } else if (lowerCmd == "quit" || lowerCmd == "q" || lowerCmd == "stop" || lowerCmd == "exit" ||
               lowerCmd == "leave") {
        handleQuit(console, shutdownCallback);
    }
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
