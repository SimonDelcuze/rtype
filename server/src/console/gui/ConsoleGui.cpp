#include "console/gui/ConsoleGui.hpp"

#include "console/ServerConsole.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace
{
    const char* RESET       = "\033[0m";
    const char* BOLD        = "\033[1m";
    const char* DIM         = "\033[2m";

    const char* FG_CYAN    = "\033[36m";
    const char* FG_MAGENTA = "\033[35m";
    const char* FG_GREEN   = "\033[32m";
    const char* FG_BLUE    = "\033[34m";
    const char* FG_YELLOW  = "\033[33m";
    const char* FG_RED     = "\033[31m";
    const char* FG_WHITE   = "\033[37m";

    std::string pos(int l, int c)
    {
        return "\033[" + std::to_string(l) + ";" + std::to_string(c) + "H";
    }

    std::string clr()
    {
        return "\033[K";
    }

    std::string repeatString(const std::string& s, int count)
    {
        std::string res;
        for (int i = 0; i < count; ++i)
            res += s;
        return res;
    }

    std::string formatTime(std::chrono::steady_clock::duration d)
    {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(d).count();
        int h     = secs / 3600;
        int m     = (secs % 3600) / 60;
        int s     = secs % 60;
        std::ostringstream ss;
        ss << std::setfill('0') << std::setw(2) << h << ":" << std::setfill('0') << std::setw(2) << m << ":"
           << std::setfill('0') << std::setw(2) << s;
        return ss.str();
    }

    std::string formatBytes(std::size_t bytes)
    {
        if (bytes < 1024)
            return std::to_string(bytes) + " B";
        if (bytes < 1024 * 1024)
            return std::to_string(bytes / 1024) + " KB";
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        return ss.str();
    }
}

ConsoleGui::ConsoleGui()
{
    updateDimensions();
}

ConsoleGui::~ConsoleGui()
{
}

void ConsoleGui::updateDimensions()
{
#ifndef _WIN32
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    width_  = w.ws_col;
    height_ = w.ws_row;
#endif
}

void ConsoleGui::render(const ServerStats& stats, const std::deque<float>& bandwidthHistory,
                        const std::deque<std::string>& logs, const std::deque<std::string>& adminLogs,
                        const std::string& inputBuffer, int logFilterRoom,
                        std::chrono::steady_clock::time_point startTime, float maxBandwidth)
{
    updateDimensions();

    std::ostringstream frame;
    drawHeader(frame, stats, startTime);
    drawStats(frame, stats);
    drawGraph(frame, bandwidthHistory, maxBandwidth);
    drawLogs(frame, logs, logFilterRoom);
    drawAdmin(frame, adminLogs, inputBuffer);

    std::cout << frame.str();
    std::cout.flush();
}

void ConsoleGui::drawHeader(std::ostringstream& frame, const ServerStats& stats,
                            std::chrono::steady_clock::time_point startTime)
{
    std::string uptime = formatTime(std::chrono::steady_clock::now() - startTime);
    std::string title  = " R-TYPE SERVER ";
    std::string right  = " [ " + uptime + " | " + std::to_string(stats.roomCount) + " ROOMS | " +
                        std::to_string(stats.clientCount) + " CLIENTS ] ";

    frame << pos(1, 1) << BOLD << FG_MAGENTA << " " << title << RESET;
    int space = width_ - static_cast<int>(title.length()) - static_cast<int>(right.length()) - 2;
    if (space > 0)
        frame << repeatString(" ", space);
    frame << BOLD << FG_WHITE << right << RESET << clr() << "\n";
    frame << pos(2, 1) << DIM << repeatString("━", width_) << RESET << clr() << "\n";
}

void ConsoleGui::drawStats(std::ostringstream& frame, const ServerStats& stats)
{
    frame << pos(3, 1) << "  " << BOLD << FG_GREEN << "IN: " << RESET << std::setw(10) << std::left
          << formatBytes(stats.bytesIn) << BOLD << FG_CYAN << " PACKETS: " << RESET << std::setw(8) << std::left
          << stats.packetsIn << " | " << BOLD << FG_BLUE << "OUT: " << RESET << std::setw(10) << std::left
          << formatBytes(stats.bytesOut) << BOLD << FG_CYAN << " PACKETS: " << RESET << std::setw(8) << std::left
          << stats.packetsOut << " | " << BOLD << FG_RED << "LOSS: " << RESET << stats.packetsLost << clr() << "\n";
    frame << pos(4, 1) << clr() << "\n";
}

void ConsoleGui::drawGraph(std::ostringstream& frame, const std::deque<float>& bandwidthHistory, float maxBandwidth)
{
    int graphHeight      = 4;
    int startLine        = 5;
    const char* blocks[] = {" ", " ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};

    frame << pos(startLine, 1) << "  " << DIM << "BANDWIDTH USAGE" << RESET << clr() << "\n";

    int graphStartCol = 12;
    int graphWidth    = width_ - graphStartCol - 2;

    for (int h = graphHeight; h > 0; --h) {
        frame << pos(startLine + (graphHeight - h + 1), 1);

        if (h == graphHeight) {
            frame << std::setw(10) << std::right << formatBytes(static_cast<std::size_t>(maxBandwidth)) << "/s" << DIM
                  << " ┐ " << RESET;
        } else if (h == 1) {
            frame << std::setw(10) << std::right << "0 B/s" << DIM << " ┘ " << RESET;
        } else if (h == (graphHeight / 2) + 1) {
            frame << std::setw(10) << std::right << formatBytes(static_cast<std::size_t>(maxBandwidth / 2)) << "/s"
                  << DIM << " ┤ " << RESET;
        } else {
            frame << std::string(10, ' ') << DIM << " │ " << RESET;
        }

        std::size_t historySize = bandwidthHistory.size();
        for (int i = 0; i < graphWidth; ++i) {
            std::size_t historyIdx = 0;
            if (historySize > static_cast<std::size_t>(graphWidth))
                historyIdx = historySize - graphWidth + i;
            else if (i < (graphWidth - static_cast<int>(historySize))) {
                frame << " ";
                continue;
            } else
                historyIdx = i - (graphWidth - historySize);

            float val        = bandwidthHistory[historyIdx] / maxBandwidth;
            int currentLevel = static_cast<int>(val * graphHeight * 8) - (h - 1) * 8;

            if (currentLevel <= 0)
                frame << " ";
            else if (currentLevel >= 8)
                frame << FG_CYAN << blocks[8] << RESET;
            else
                frame << FG_CYAN << blocks[currentLevel] << RESET;
        }
        frame << clr() << "\n";
    }
    frame << pos(startLine + graphHeight + 1, 1) << "  " << DIM << "└" << repeatString("─", graphWidth) << RESET
          << clr() << "\n";
}

void ConsoleGui::drawLogs(std::ostringstream& frame, const std::deque<std::string>& logs, int logFilterRoom)
{
    int topOffset = 12;
    int logLines  = height_ - topOffset - 10;
    if (logLines <= 0)
        return;

    frame << pos(topOffset, 1) << BOLD << FG_YELLOW << " LOGS ";
    if (logFilterRoom >= 0) {
        frame << DIM << "(Room " << logFilterRoom << ")" << RESET;
    }
    frame << RESET << clr() << "\n";
    frame << pos(topOffset + 1, 1) << DIM << repeatString("─", width_) << RESET << clr() << "\n";

    std::deque<std::string> filteredLogs;
    if (logFilterRoom < 0) {
        filteredLogs = logs;
    } else {
        std::string prefix = "[Room " + std::to_string(logFilterRoom) + "]";
        for (const auto& log : logs) {
            if (log.find(prefix) != std::string::npos) {
                filteredLogs.push_back(log);
            }
        }
    }

    std::size_t startIdx =
        std::max(static_cast<std::size_t>(0), filteredLogs.size() - static_cast<std::size_t>(logLines));
    for (std::size_t i = 0; i < static_cast<std::size_t>(logLines); ++i) {
        frame << pos(topOffset + 2 + i, 1);
        if (startIdx + i < filteredLogs.size()) {
            std::string line = filteredLogs[startIdx + i];
            if (line.size() > static_cast<std::size_t>(width_) - 4)
                line = line.substr(0, width_ - 7) + "...";
            if (!line.empty() && line.back() == '\n')
                line.pop_back();
            std::string color = RESET;
            if (line.find("[ERROR]") != std::string::npos)
                color = FG_RED;
            else if (line.find("[WARN]") != std::string::npos)
                color = FG_YELLOW;
            frame << " " << color << "│ " << RESET << DIM << line << RESET;
        }
        frame << clr() << (i == static_cast<std::size_t>(logLines) - 1 ? "" : "\n");
    }
}

void ConsoleGui::drawAdmin(std::ostringstream& frame, const std::deque<std::string>& adminLogs,
                           const std::string& inputBuffer)
{
    int adminLines = 5;
    int startRow   = height_ - adminLines - 3;

    frame << pos(startRow, 1) << BOLD << FG_MAGENTA << " ADMIN OUTPUT " << RESET << clr() << "\n";
    frame << pos(startRow + 1, 1) << DIM << repeatString("─", width_) << RESET << clr() << "\n";

    for (int i = 0; i < adminLines; ++i) {
        frame << pos(startRow + 2 + i, 1);
        std::size_t logSize = adminLogs.size();
        if (logSize > 0) {
            int idx = static_cast<int>(logSize) - adminLines + i;
            if (idx >= 0) {
                frame << " " << FG_MAGENTA << "│ " << RESET << DIM << adminLogs[idx] << RESET;
            }
        }
        frame << clr() << "\n";
    }

    frame << pos(height_ - 1, 1) << DIM << repeatString("─", width_) << RESET << clr() << "\n";
    frame << pos(height_, 1) << BOLD << FG_MAGENTA << " ADMIN> " << RESET << inputBuffer << "\033[?25h" << clr();
}
