#include "console/commands/ConsoleCommands.hpp"

#include "Logger.hpp"
#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"
#include "console/commands/LogCommands.hpp"
#include "console/commands/RoomCommands.hpp"
#include "console/commands/SystemCommands.hpp"
#include "console/commands/TagsCommands.hpp"
#include "core/Session.hpp"
#include "game/GameInstanceManager.hpp"

ConsoleCommands::ConsoleCommands(GameInstanceManager* instanceManager, LobbyManager* lobbyManager,
                                 ServerConsole* console)
    : instanceManager_(instanceManager), lobbyManager_(lobbyManager), console_(console)
{}

void ConsoleCommands::setCommandHandler(CommandHandler handler)
{
    commandHandler_ = std::move(handler);
}

void ConsoleCommands::setBroadcastCallback(std::function<void(const std::string&)> callback)
{
    broadcastCallback_ = std::move(callback);
}

void ConsoleCommands::setShutdownCallback(std::function<void()> callback)
{
    shutdownCallback_ = std::move(callback);
}

void ConsoleCommands::processCommand(const std::string& cmd)
{
    if (cmd.empty())
        return;

    console_->addAdminLog("> " + cmd);

    if (SystemCommands::handleCommand(console_, cmd, shutdownCallback_, broadcastCallback_))
        return;

    if (RoomCommands::handleCommand(console_, instanceManager_, lobbyManager_, cmd))
        return;

    if (LogCommands::handleCommand(console_, cmd))
        return;

    if (TagsCommands::handleCommand(console_, cmd))
        return;

    if (commandHandler_) {
        commandHandler_(cmd);
        return;
    }

    console_->addAdminLog("[Error] Unknown command: " + cmd);
}
