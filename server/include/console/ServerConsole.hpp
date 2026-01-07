#pragma once

#include "console/commands/ConsoleCommands.hpp"
#include "console/gui/ConsoleGui.hpp"
#include "game/GameInstanceManager.hpp"

#include <chrono>
#include <deque>
#include <memory>
#include <string>

#ifndef _WIN32
#include <termios.h>
#endif

struct ServerStats
{
    std::size_t bytesIn{0};
    std::size_t bytesOut{0};
    std::size_t packetsIn{0};
    std::size_t packetsOut{0};
    std::size_t packetsLost{0};
    std::size_t roomCount{0};
    std::size_t clientCount{0};
};

class LobbyManager;

class ServerConsole
{
  public:
    explicit ServerConsole(GameInstanceManager* instanceManager, LobbyManager* lobbyManager);
    ~ServerConsole();

    void update(const ServerStats& stats);
    void addLog(const std::string& log);
    void addLog(std::uint32_t roomId, const std::string& log);
    void addAdminLog(const std::string& msg);

    void render();
    void handleInput();

    void setCommandHandler(ConsoleCommands::CommandHandler handler);
    void setShutdownCallback(std::function<void()> callback);
    void setBroadcastCallback(std::function<void(const std::string&)> callback);

    void setLogFilterRoom(int roomId)
    {
        logFilterRoom_ = roomId;
    }
    int getLogFilterRoom() const
    {
        return logFilterRoom_;
    }

    const ServerStats& getCurrentStats() const
    {
        return currentStats_;
    }

  private:
    GameInstanceManager* instanceManager_{nullptr};
    LobbyManager* lobbyManager_{nullptr};

    std::unique_ptr<ConsoleGui> gui_;
    std::unique_ptr<ConsoleCommands> commands_;

    ServerStats currentStats_{};
    std::deque<float> bandwidthHistory_;
    std::deque<std::string> logs_;
    std::deque<std::string> adminLogs_;

    std::chrono::steady_clock::time_point lastUpdate_;
    std::chrono::steady_clock::time_point startTime_;

    float maxBandwidth_{1.0f};
    std::string inputBuffer_;
    std::int32_t logFilterRoom_{-1};

#ifndef _WIN32
    struct termios origTermios_;
#endif
};
