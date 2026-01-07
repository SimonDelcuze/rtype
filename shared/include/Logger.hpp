#pragma once

#include <atomic>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

class Logger
{
  public:
    static Logger& instance();

    void setVerbose(bool enabled);
    void loadTagConfig(const std::string& configPath);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void verbose(const std::string& message);

    void addBytesSent(std::size_t bytes);
    void addBytesReceived(std::size_t bytes);
    void addPacketSent();
    void addPacketReceived();
    void addPacketDropped();

    std::size_t getTotalBytesSent() const
    {
        return _totalBytesSent;
    }
    std::size_t getTotalBytesReceived() const
    {
        return _totalBytesReceived;
    }
    std::size_t getTotalPacketsSent() const
    {
        return _totalPacketsSent;
    }
    std::size_t getTotalPacketsReceived() const
    {
        return _totalPacketsReceived;
    }
    std::size_t getTotalPacketsDropped() const
    {
        return _totalPacketsDropped;
    }

    void setConsoleOutputEnabled(bool enabled);
    void setPostLogCallback(std::function<void(const std::string&)> callback);

    void logNetworkStats();

    // Tag management
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    std::vector<std::string> getEnabledTags() const;
    std::vector<std::string> getAllKnownTags() const;
    bool hasTag(const std::string& tag) const;

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

  private:
    Logger();
    ~Logger();

    void log(const std::string& level, const std::string& message, bool alwaysConsole);
    bool isTagEnabled(const std::string& message) const;
    static std::string extractTag(const std::string& message);

    std::ofstream _file;
    mutable std::mutex _mutex;
    bool _verbose;
    bool _consoleEnabled{true};
    std::unordered_set<std::string> _enabledTags;
    bool _tagFilterActive;
    std::function<void(const std::string&)> _postLogCallback;

    std::atomic<std::size_t> _totalBytesSent{0};
    std::atomic<std::size_t> _totalBytesReceived{0};
    std::atomic<std::size_t> _totalPacketsSent{0};
    std::atomic<std::size_t> _totalPacketsReceived{0};
    std::atomic<std::size_t> _totalPacketsDropped{0};
};
