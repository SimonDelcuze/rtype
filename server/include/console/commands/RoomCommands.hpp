#pragma once

#include <string>

class ServerConsole;
class GameInstanceManager;

class RoomCommands
{
  public:
    static void handleCommand(ServerConsole* console, GameInstanceManager* instanceManager, const std::string& cmd);

  private:
    static void handleList(ServerConsole* console, GameInstanceManager* instanceManager);
    static void handleKill(ServerConsole* console, GameInstanceManager* instanceManager, const std::string& idArg);
};
