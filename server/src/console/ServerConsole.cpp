#include "console/ServerConsole.hpp"

#include "Logger.hpp"

#include <iostream>
#ifndef _WIN32
#include <poll.h>
#include <unistd.h>
#endif

ServerConsole::ServerConsole(GameInstanceManager* instanceManager)
    : instanceManager_(instanceManager), lastUpdate_(std::chrono::steady_clock::now()),
      startTime_(std::chrono::steady_clock::now())
{
    std::cout << "\033[2J" << "\033[?25l";

    gui_      = std::make_unique<ConsoleGui>();
    commands_ = std::make_unique<ConsoleCommands>(instanceManager, this);

    gui_->updateDimensions();

    for (int i = 0; i < 200; ++i)
        bandwidthHistory_.push_back(0.0f);

#ifndef _WIN32
    tcgetattr(STDIN_FILENO, &origTermios_);
    struct termios raw = origTermios_;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif

    Logger::instance().setConsoleOutputEnabled(false);
    Logger::instance().setPostLogCallback([this](const std::string& msg) { addLog(msg); });
}

ServerConsole::~ServerConsole()
{
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios_);
#endif
    Logger::instance().setConsoleOutputEnabled(true);
    Logger::instance().setPostLogCallback(nullptr);
    std::cout << "\033[?25h" << "\033[0m" << "\033[2J" << "\033[H";
}

void ServerConsole::update(const ServerStats& stats)
{
    auto now  = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - lastUpdate_).count();

    if (dt >= 0.1) {
        if (currentStats_.bytesOut != 0 || stats.bytesOut != 0) {
            double diff        = static_cast<double>(stats.bytesOut) - static_cast<double>(currentStats_.bytesOut);
            double bytesPerSec = diff / dt;
            if (bytesPerSec < 0)
                bytesPerSec = 0;
            bandwidthHistory_.pop_front();
            bandwidthHistory_.push_back(static_cast<float>(bytesPerSec));
        } else {
            bandwidthHistory_.pop_front();
            bandwidthHistory_.push_back(0.0f);
        }

        maxBandwidth_ = 1024.0f;
        for (float val : bandwidthHistory_) {
            if (val > maxBandwidth_)
                maxBandwidth_ = val;
        }

        currentStats_ = stats;
        lastUpdate_   = now;
    }
}

void ServerConsole::addLog(const std::string& log)
{
    logs_.push_back(log);
    if (logs_.size() > 500) {
        logs_.pop_front();
    }
}

void ServerConsole::addLog(std::uint32_t roomId, const std::string& log)
{
    std::string prefixed = "[Room " + std::to_string(roomId) + "] " + log;
    logs_.push_back(prefixed);
    if (logs_.size() > 500) {
        logs_.pop_front();
    }
}

void ServerConsole::addAdminLog(const std::string& msg)
{
    adminLogs_.push_back(msg);
    if (adminLogs_.size() > 50) {
        adminLogs_.pop_front();
    }
}

void ServerConsole::render()
{
    if (gui_) {
        gui_->render(currentStats_, bandwidthHistory_, logs_, adminLogs_, inputBuffer_, logFilterRoom_, startTime_,
                     maxBandwidth_);
    }
}

void ServerConsole::handleInput()
{
#ifndef _WIN32
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    if (poll(&pfd, 1, 0) > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == '\n' || c == '\r') {
                if (!inputBuffer_.empty() && commands_) {
                    commands_->processCommand(inputBuffer_);
                    inputBuffer_.clear();
                }
            } else if (c == 127 || c == 8) {
                if (!inputBuffer_.empty())
                    inputBuffer_.pop_back();
            } else if (c >= 32 && c <= 126) {
                inputBuffer_ += c;
            }
        }
    }
#endif
}

void ServerConsole::setCommandHandler(ConsoleCommands::CommandHandler handler)
{
    if (commands_) {
        commands_->setCommandHandler(std::move(handler));
    }
}

void ServerConsole::setShutdownCallback(std::function<void()> callback)
{
    if (commands_) {
        commands_->setShutdownCallback(std::move(callback));
    }
}
