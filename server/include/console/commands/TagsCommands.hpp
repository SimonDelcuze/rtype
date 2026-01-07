#pragma once

#include <string>

class ServerConsole;

class TagsCommands
{
  public:
    static void handleCommand(ServerConsole* console, const std::string& cmd);

  private:
    static void handleList(ServerConsole* console);
    static void handleAdd(ServerConsole* console, const std::string& tagArg);
    static void handleRemove(ServerConsole* console, const std::string& tagArg);
    static void handleHelp(ServerConsole* console);
};
