#pragma once

#include <chrono>
#include <deque>
#include <string>
#include <termios.h>
#include <vector>

struct NetworkStats
{
    size_t bytesIn;
    size_t bytesOut;
    size_t packetsIn;
    size_t packetsOut;
    size_t packetsLost;
};

class NetworkTui
{
  public:
    NetworkTui(bool showNetwork = false, bool showAdmin = false);
    ~NetworkTui();

    void update(const NetworkStats& stats);
    void setClientCount(size_t count)
    {
        _clientCount = count;
    }
    void addLog(const std::string& log);
    void addAdminLog(const std::string& msg);
    void render();
    void handleInput();

  private:
    void drawHeader(std::ostringstream& frame);
    void drawStats(std::ostringstream& frame);
    void drawGraph(std::ostringstream& frame);
    void drawLogs(std::ostringstream& frame);
    void drawAdmin(std::ostringstream& frame);
    void processCommand(const std::string& cmd);
    void resetCursor();

    NetworkStats _currentStats{0, 0, 0, 0, 0};
    NetworkStats _lastStats{0, 0, 0, 0, 0};
    std::deque<float> _bandwidthHistory;
    std::deque<std::string> _logs;
    std::deque<std::string> _adminLogs;
    std::chrono::steady_clock::time_point _lastUpdate;
    std::chrono::steady_clock::time_point _startTime;

    int _width{80};
    int _height{24};
    float _maxBandwidth{1.0f};
    size_t _clientCount{0};
    bool _showNetwork{false};
    bool _adminMode{false};
    std::string _inputBuffer;
    struct termios _origTermios;
};
