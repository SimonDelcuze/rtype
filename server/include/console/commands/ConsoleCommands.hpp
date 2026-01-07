#pragma once

#include <deque>
#include <functional>
#include <string>

class GameInstanceManager;
class ServerConsole;

class ConsoleCommands
{
  public:
    using CommandHandler = std::function<void(const std::string&)>;

    ConsoleCommands(GameInstanceManager* instanceManager, ServerConsole* console);

    void processCommand(const std::string& cmd);
    void setCommandHandler(CommandHandler handler);
    void setShutdownCallback(std::function<void()> callback);

  private:
    GameInstanceManager* instanceManager_;
    ServerConsole* console_;
    CommandHandler commandHandler_;
    std::function<void()> shutdownCallback_;
};
