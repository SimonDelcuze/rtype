#include "console/commands/ConsoleCommands.hpp"

#include "console/ServerConsole.hpp"
#include "console/commands/CommandUtils.hpp"
#include "console/commands/LogCommands.hpp"
#include "console/commands/RoomCommands.hpp"
#include "console/commands/SystemCommands.hpp"
#include "console/commands/TagsCommands.hpp"

ConsoleCommands::ConsoleCommands(GameInstanceManager* instanceManager, ServerConsole* console)
    : instanceManager_(instanceManager), console_(console)
{
}

void ConsoleCommands::setCommandHandler(CommandHandler handler)
{
    commandHandler_ = std::move(handler);
}

void ConsoleCommands::setShutdownCallback(std::function<void()> callback)
{
    shutdownCallback_ = std::move(callback);
}

void ConsoleCommands::processCommand(const std::string& cmd)
{
    console_->addAdminLog("> " + cmd);


    SystemCommands::handleCommand(console_, cmd, shutdownCallback_);
    RoomCommands::handleCommand(console_, instanceManager_, cmd);
    LogCommands::handleCommand(console_, cmd);
    TagsCommands::handleCommand(console_, cmd);


    if (commandHandler_) {











        std::string lowerCmd = CommandUtils::toLower(cmd);

        bool handled = false;

        if (lowerCmd == "help" || lowerCmd == "ping" ||
            lowerCmd == "quit" || lowerCmd == "q" || lowerCmd == "stop" || lowerCmd == "exit" || lowerCmd == "leave") {
            handled = true;
        } else if (lowerCmd == "rooms" || lowerCmd == "tags list" || CommandUtils::startsWithIgnoreCase(cmd, "kill ") ||
                   CommandUtils::startsWithIgnoreCase(cmd, "tags ")) {
            handled = true;
        } else if (CommandUtils::startsWithIgnoreCase(cmd, "logs ")) {
            handled = true;
        }

        if (!handled && commandHandler_) {
            commandHandler_(cmd);
            return;
        }

        if(!handled) {
            console_->addAdminLog("[Error] Unknown command: " + cmd);
        }
    }
}
