#pragma once

#include <deque>
#include <functional>
#include <string>

class GameInstanceManager;
class ServerConsole;

class LobbyManager;

class ConsoleCommands
{
  public:
    using CommandHandler = std::function<void(const std::string&)>;

    ConsoleCommands(GameInstanceManager* instanceManager, LobbyManager* lobbyManager, ServerConsole* console);

    void processCommand(const std::string& cmd);
    void setCommandHandler(CommandHandler handler);
    void setShutdownCallback(std::function<void()> callback);
    void setBroadcastCallback(std::function<void(const std::string&)> callback);

  private:
    GameInstanceManager* instanceManager_{nullptr};
    LobbyManager* lobbyManager_{nullptr};
    ServerConsole* console_{nullptr};
    CommandHandler commandHandler_;
    std::function<void()> shutdownCallback_;
    std::function<void(const std::string&)> broadcastCallback_;
};
