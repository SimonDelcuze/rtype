#pragma once

#include <functional>
#include <string>

class ServerConsole;

class SystemCommands
{
  public:
    static void handleCommand(ServerConsole* console, const std::string& cmd,
                              const std::function<void()>& shutdownCallback);

  private:
    static void handleHelp(ServerConsole* console);
    static void handleStats(ServerConsole* console);
    static void handleQuit(ServerConsole* console, const std::function<void()>& shutdownCallback);
    static void handlePing(ServerConsole* console);
};
