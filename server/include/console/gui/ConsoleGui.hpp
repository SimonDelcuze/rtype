#pragma once

#include <chrono>
#include <deque>
#include <sstream>
#include <string>

struct ServerStats;

class ConsoleGui
{
  public:
    ConsoleGui();
    ~ConsoleGui();

    void updateDimensions();
    void render(const ServerStats& stats, const std::deque<float>& bandwidthHistory, const std::deque<std::string>& logs,
                const std::deque<std::string>& adminLogs, const std::string& inputBuffer, int logFilterRoom,
                std::chrono::steady_clock::time_point startTime, float maxBandwidth);

  private:
    void drawHeader(std::ostringstream& frame, const ServerStats& stats, std::chrono::steady_clock::time_point startTime);
    void drawStats(std::ostringstream& frame, const ServerStats& stats);
    void drawGraph(std::ostringstream& frame, const std::deque<float>& bandwidthHistory, float maxBandwidth);
    void drawLogs(std::ostringstream& frame, const std::deque<std::string>& logs, int logFilterRoom);
    void drawAdmin(std::ostringstream& frame, const std::deque<std::string>& adminLogs, const std::string& inputBuffer);

    int width_{80};
    int height_{24};
};
