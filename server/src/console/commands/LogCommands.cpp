#include "console/commands/LogCommands.hpp"

#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"

void LogCommands::handleCommand(ServerConsole* console, const std::string& cmd)
{
    if (CommandUtils::startsWithIgnoreCase(cmd, "logs ")) {
        handleFilter(console, cmd.substr(5));
    }
}

void LogCommands::handleFilter(ServerConsole* console, const std::string& arg)
{
    std::string lowerArg = CommandUtils::toLower(arg);
    if (lowerArg == "all") {
        console->setLogFilterRoom(-1);
        console->addAdminLog("[Logs] Showing all logs");
    } else {
        try {
            int roomId = std::stoi(arg);
            console->setLogFilterRoom(roomId);
            console->addAdminLog("[Logs] Filtering to room " + std::to_string(roomId));
        } catch (...) {
            console->addAdminLog("[Error] Invalid room ID: " + arg);
        }
    }
}
