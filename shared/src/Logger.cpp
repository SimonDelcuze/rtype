#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger() : _verbose(false)
{
    const std::filesystem::path directory("logs");
    const std::filesystem::path filePath = directory / "server.log";

    try {
        std::filesystem::create_directories(directory);
    } catch (...) {  // NOLINT(bugprone-empty-catch)
    }

    _file.open(filePath, std::ios::app);
}

Logger::~Logger()
{
    if (_file.is_open()) {
        _file.flush();
        _file.close();
    }
}

void Logger::setVerbose(bool enabled)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _verbose = enabled;
}

void Logger::info(const std::string& message)
{
    log("INFO", message, false);
}

void Logger::warn(const std::string& message)
{
    log("WARN", message, true);
}

void Logger::error(const std::string& message)
{
    log("ERROR", message, true);
}

void Logger::log(const std::string& level, const std::string& message, bool alwaysConsole)
{
    const auto now         = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::lock_guard<std::mutex> lock(_mutex);

    std::tm timeInfo{};
    std::tm* localTime = std::localtime(&time);
    if (localTime != nullptr) {
        timeInfo = *localTime;
    }

    std::ostringstream stream;
    stream << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    const std::string timestamp = stream.str();
    const std::string line      = "[" + timestamp + "][" + level + "] " + message + '\n';

    if (_file.is_open()) {
        _file << line;
        _file.flush();
    }

    const bool consoleEnabled = alwaysConsole || _verbose;
    if (!consoleEnabled) {
        return;
    }

    if (level == "ERROR") {
        std::cerr << line;
    } else {
        std::cout << line;
    }
}
