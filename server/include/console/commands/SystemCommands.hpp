#pragma once

#include <functional>
#include <string>

class ServerConsole;

class SystemCommands
{
  public:
    static bool handleCommand(ServerConsole* console, const std::string& cmd,
                              const std::function<void()>& shutdownCallback,
                              const std::function<void(const std::string&)>& broadcastCallback);

  private:
    static void handleHelp(ServerConsole* console);
    static void handleStats(ServerConsole* console);
    static void handleQuit(ServerConsole* console, const std::function<void()>& shutdownCallback);
    static void handlePing(ServerConsole* console);
};
