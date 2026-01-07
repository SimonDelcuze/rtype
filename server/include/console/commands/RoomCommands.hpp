#pragma once

#include <string>

class ServerConsole;
class GameInstanceManager;

class LobbyManager;

class RoomCommands
{
  public:
    static bool handleCommand(ServerConsole* console, GameInstanceManager* instanceManager, LobbyManager* lobbyManager,
                              const std::string& cmd);

  private:
    static void handleList(ServerConsole* console, GameInstanceManager* instanceManager);
    static void handleKill(ServerConsole* console, GameInstanceManager* instanceManager, LobbyManager* lobbyManager,
                           const std::string& idArg);
};
