#pragma once

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_set>

class Logger
{
  public:
    static Logger& instance();

    void setVerbose(bool enabled);
    void loadTagConfig(const std::string& configPath);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    void addBytesSent(std::size_t bytes);
    void addBytesReceived(std::size_t bytes);
    void addPacketDropped();
    void logNetworkStats();

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

  private:
    Logger();
    ~Logger();

    void log(const std::string& level, const std::string& message, bool alwaysConsole);
    bool isTagEnabled(const std::string& message) const;
    static std::string extractTag(const std::string& message);

    std::ofstream _file;
    std::mutex _mutex;
    bool _verbose;
    std::unordered_set<std::string> _enabledTags;
    bool _tagFilterActive;

    std::atomic<std::size_t> _totalBytesSent{0};
    std::atomic<std::size_t> _totalBytesReceived{0};
    std::atomic<std::size_t> _totalPacketsDropped{0};
};
