#pragma once

#include <string>

class ServerConsole;

class LogCommands
{
  public:
    static void handleCommand(ServerConsole* console, const std::string& cmd);

  private:
    static void handleFilter(ServerConsole* console, const std::string& arg);
};
