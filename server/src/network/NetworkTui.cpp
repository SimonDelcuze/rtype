#include "network/NetworkTui.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace
{
    const char* HIDE_CURSOR = "\033[?25l";
    const char* SHOW_CURSOR = "\033[?25h";
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

    std::string formatBytes(size_t bytes)
    {
        if (bytes < 1024)
            return std::to_string(bytes) + " B";
        if (bytes < 1024 * 1024)
            return std::to_string(bytes / 1024) + " KB";
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        return ss.str();
    }
} // namespace

NetworkTui::NetworkTui(bool showNetwork, bool showAdmin)
    : _lastUpdate(std::chrono::steady_clock::now()), _startTime(std::chrono::steady_clock::now()),
      _showNetwork(showNetwork), _adminMode(showAdmin)
{
    std::cout << "\033[2J" << HIDE_CURSOR;

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    _width  = w.ws_col;
    _height = w.ws_row;

    for (int i = 0; i < _width; ++i)
        _bandwidthHistory.push_back(0.0f);

    if (_adminMode) {
        tcgetattr(STDIN_FILENO, &_origTermios);
        struct termios raw = _origTermios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
}

NetworkTui::~NetworkTui()
{
    if (_adminMode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &_origTermios);
    }
    std::cout << SHOW_CURSOR << RESET << "\033[2J" << "\033[H";
}

void NetworkTui::handleInput()
{
    if (!_adminMode)
        return;

    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    if (poll(&pfd, 1, 0) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n' || c == '\r') {
                if (!_inputBuffer.empty()) {
                    processCommand(_inputBuffer);
                    _inputBuffer.clear();
                }
            } else if (c == 127 || c == 8) {
                if (!_inputBuffer.empty())
                    _inputBuffer.pop_back();
            } else if (c >= 32 && c <= 126) {
                _inputBuffer += c;
            }
        }
    }
}

void NetworkTui::processCommand(const std::string& cmd)
{
    addAdminLog("> " + cmd);
    if (cmd == "/ping") {
        addAdminLog("[Pong] Server is alive!");
    } else {
        addAdminLog("[Admin] Error: Unknown command: " + cmd);
    }
}

void NetworkTui::update(const NetworkStats& stats)
{
    auto now  = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - _lastUpdate).count();

    if (dt >= 0.1) {
        if (_currentStats.bytesOut != 0 || stats.bytesOut != 0) {
            double diff        = (double) stats.bytesOut - (double) _currentStats.bytesOut;
            double bytesPerSec = diff / dt;
            if (bytesPerSec < 0)
                bytesPerSec = 0;
            _bandwidthHistory.pop_front();
            _bandwidthHistory.push_back((float) bytesPerSec);
        } else {
            _bandwidthHistory.pop_front();
            _bandwidthHistory.push_back(0.0f);
        }

        _maxBandwidth = 1024.0f;
        for (float val : _bandwidthHistory) {
            if (val > _maxBandwidth)
                _maxBandwidth = val;
        }

        _currentStats = stats;
        _lastUpdate   = now;
    }
}

void NetworkTui::addLog(const std::string& log)
{
    _logs.push_back(log);
    if (_logs.size() > 500) {
        _logs.pop_front();
    }
}

void NetworkTui::addAdminLog(const std::string& msg)
{
    _adminLogs.push_back(msg);
    if (_adminLogs.size() > 50) {
        _adminLogs.pop_front();
    }
}

void NetworkTui::render()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_col != _width || w.ws_row != _height) {
        _width  = w.ws_col;
        _height = w.ws_row;
        std::cout << "\033[2J";
    }

    std::ostringstream frame;
    drawHeader(frame);
    if (_showNetwork) {
        drawStats(frame);
        drawGraph(frame);
    }
    drawLogs(frame);
    if (_adminMode) {
        drawAdmin(frame);
    }

    std::cout << frame.str();
    std::cout.flush();
}

void NetworkTui::drawHeader(std::ostringstream& frame)
{
    std::string uptime = formatTime(std::chrono::steady_clock::now() - _startTime);
    std::string title  = " R-TYPE ";
    std::string right  = " [ " + uptime + " | " + std::to_string(_clientCount) + " CLIENTS ] ";

    frame << pos(1, 1) << BOLD << FG_MAGENTA << " " << title << RESET;
    int space = _width - (int) title.length() - (int) right.length() - 2;
    if (space > 0)
        frame << repeatString(" ", space);
    frame << BOLD << FG_WHITE << right << RESET << clr() << "\n";
    frame << pos(2, 1) << DIM << repeatString("━", _width) << RESET << clr() << "\n";
}

void NetworkTui::drawStats(std::ostringstream& frame)
{
    frame << pos(3, 1) << "  " << BOLD << FG_GREEN << "IN: " << RESET << std::setw(10) << std::left
          << formatBytes(_currentStats.bytesIn) << BOLD << FG_CYAN << " PACKETS: " << RESET << std::setw(8) << std::left
          << _currentStats.packetsIn << " | " << BOLD << FG_BLUE << "OUT: " << RESET << std::setw(10) << std::left
          << formatBytes(_currentStats.bytesOut) << BOLD << FG_CYAN << " PACKETS: " << RESET << std::setw(8)
          << std::left << _currentStats.packetsOut << " | " << BOLD << FG_RED << "LOSS: " << RESET
          << _currentStats.packetsLost << clr() << "\n";
    frame << pos(4, 1) << clr() << "\n";
}

void NetworkTui::drawGraph(std::ostringstream& frame)
{
    int graphHeight      = 4;
    int startLine        = 5;
    const char* blocks[] = {" ", " ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};

    frame << pos(startLine, 1) << "  " << DIM << "BANDWIDTH USAGE" << RESET << clr() << "\n";

    int graphStartCol = 12;
    int graphWidth    = _width - graphStartCol - 2;

    for (int h = graphHeight; h > 0; --h) {
        frame << pos(startLine + (graphHeight - h + 1), 1);

        if (h == graphHeight) {
            frame << std::setw(10) << std::right << formatBytes((size_t) _maxBandwidth) << "/s" << DIM << " ┐ "
                  << RESET;
        } else if (h == 1) {
            frame << std::setw(10) << std::right << "0 B/s" << DIM << " ┘ " << RESET;
        } else if (h == (graphHeight / 2) + 1) {
            frame << std::setw(10) << std::right << formatBytes((size_t) (_maxBandwidth / 2)) << "/s" << DIM << " ┤ "
                  << RESET;
        } else {
            frame << std::string(10, ' ') << DIM << " │ " << RESET;
        }

        size_t historySize = _bandwidthHistory.size();
        for (int i = 0; i < graphWidth; ++i) {
            size_t historyIdx = 0;
            if (historySize > (size_t) graphWidth)
                historyIdx = historySize - graphWidth + i;
            else if (i < (graphWidth - (int) historySize)) {
                frame << " ";
                continue;
            } else
                historyIdx = i - (graphWidth - historySize);

            float val        = _bandwidthHistory[historyIdx] / _maxBandwidth;
            int currentLevel = (int) (val * graphHeight * 8) - (h - 1) * 8;

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

void NetworkTui::drawLogs(std::ostringstream& frame)
{
    int topOffset = _showNetwork ? 12 : 3;
    int logLines  = _height - topOffset - 1;
    if (_adminMode)
        logLines -= 9;
    if (logLines <= 0)
        return;

    frame << pos(topOffset, 1) << BOLD << FG_YELLOW << " LOGS " << RESET << clr() << "\n";
    frame << pos(topOffset + 1, 1) << DIM << repeatString("─", _width) << RESET << clr() << "\n";

    size_t startIdx = std::max((size_t) 0, _logs.size() - (size_t) logLines);
    for (size_t i = 0; i < (size_t) logLines; ++i) {
        frame << pos(topOffset + 2 + i, 1);
        if (startIdx + i < _logs.size()) {
            std::string line = _logs[startIdx + i];
            if (line.size() > (size_t) _width - 4)
                line = line.substr(0, _width - 7) + "...";
            if (!line.empty() && line.back() == '\n')
                line.pop_back();
            std::string color = RESET;
            if (line.find("[ERROR]") != std::string::npos)
                color = FG_RED;
            else if (line.find("[WARN]") != std::string::npos)
                color = FG_YELLOW;
            frame << " " << color << "│ " << RESET << DIM << line << RESET;
        }
        frame << clr() << (i == (size_t) logLines - 1 ? "" : "\n");
    }
}

void NetworkTui::drawAdmin(std::ostringstream& frame)
{
    int adminLines = 5;
    int startRow   = _height - adminLines - 3;

    frame << pos(startRow, 1) << BOLD << FG_MAGENTA << " ADMIN OUTPUT " << RESET << clr() << "\n";
    frame << pos(startRow + 1, 1) << DIM << repeatString("─", _width) << RESET << clr() << "\n";

    for (int i = 0; i < adminLines; ++i) {
        frame << pos(startRow + 2 + i, 1);
        size_t logSize = _adminLogs.size();
        if (logSize > 0) {
            int idx = (int) logSize - adminLines + i;
            if (idx >= 0) {
                frame << " " << FG_MAGENTA << "│ " << RESET << DIM << _adminLogs[idx] << RESET;
            }
        }
        frame << clr() << "\n";
    }

    frame << pos(_height - 1, 1) << DIM << repeatString("─", _width) << RESET << clr() << "\n";
    frame << pos(_height, 1) << BOLD << FG_MAGENTA << " ADMIN> " << RESET << _inputBuffer << SHOW_CURSOR << clr();
}

void NetworkTui::resetCursor() {}
