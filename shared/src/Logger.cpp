#include "Logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger() : _verbose(false), _tagFilterActive(false)
{
    const std::filesystem::path directory("logs");
    const std::filesystem::path filePath = directory / "server.log";

    try {
        std::filesystem::create_directories(directory);
    } catch (...) {
    }

    _file.open(filePath, std::ios::app);
}

Logger::~Logger()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_file.is_open()) {
        _file.flush();
        _file.close();
    }
    for (auto& [id, file] : _roomFiles) {
        if (file && file->is_open()) {
            file->flush();
            file->close();
        }
    }
    _roomFiles.clear();
}

void Logger::setVerbose(bool enabled)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _verbose = enabled;
}

void Logger::loadTagConfig(const std::string& configPath)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _enabledTags.clear();
    _tagFilterActive = false;

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        size_t start = line.find('[');
        size_t end   = line.find(']');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string tag = line.substr(start, end - start + 1);
            _enabledTags.insert(tag);
        }
    }
    _tagFilterActive = !_enabledTags.empty();

    if (_verbose) {
        std::cout << "[Logger] Loaded " << _enabledTags.size() << " tags from " << configPath << "\n";
        for (const auto& tag : _enabledTags) {
            std::cout << "[Logger]   - " << tag << "\n";
        }
    }
}

std::string Logger::extractTag(const std::string& message)
{
    if (message.empty() || message[0] != '[') {
        return "";
    }
    size_t end = message.find(']');
    if (end == std::string::npos) {
        return "";
    }
    return message.substr(0, end + 1);
}

bool Logger::isTagEnabled(const std::string& message) const
{
    if (!_tagFilterActive) {
        return true;
    }
    std::string tag = extractTag(message);
    if (tag.empty()) {
        return true;
    }
    return _enabledTags.contains(tag);
}

void Logger::info(const std::string& message)
{
    log(-1, "INFO", message, false);
}

void Logger::warn(const std::string& message)
{
    log(-1, "WARN", message, true);
}

void Logger::error(const std::string& message)
{
    log(-1, "ERROR", message, true);
}

void Logger::verbose(const std::string& message)
{
    log(-1, "VERBOSE", message, false);
}

void Logger::logToRoom(int roomId, const std::string& level, const std::string& message)
{
    log(roomId, level, message, false);
}

void Logger::addBytesSent(std::size_t bytes)
{
    _totalBytesSent += bytes;
}

void Logger::addBytesReceived(std::size_t bytes)
{
    _totalBytesReceived += bytes;
}

void Logger::addPacketSent()
{
    _totalPacketsSent++;
}

void Logger::addPacketReceived()
{
    _totalPacketsReceived++;
}

void Logger::addPacketDropped()
{
    _totalPacketsDropped++;
}

void Logger::setConsoleOutputEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _consoleEnabled = enabled;
}

void Logger::setPostLogCallback(std::function<void(const std::string&)> callback)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _postLogCallback = std::move(callback);
}

void Logger::logNetworkStats()
{
    std::size_t sent     = _totalBytesSent.exchange(0);
    std::size_t received = _totalBytesReceived.exchange(0);
    std::size_t dropped  = _totalPacketsDropped.exchange(0);

    info("[Net] Network Stats (last 5s): Sent=" + std::to_string(sent) +
         " bytes, Received=" + std::to_string(received) + " bytes, Dropped=" + std::to_string(dropped) + " packets");
}

void Logger::log(int roomId, const std::string& level, const std::string& message, bool alwaysConsole)
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

    std::string roomTag;
    if (roomId >= 0) {
        roomTag = "[Room " + std::to_string(roomId) + "]";
    } else {
        roomTag = "[System]";
    }

    const std::string line = "[" + timestamp + "]" + roomTag + "[" + level + "] " + message + '\n';

    if (_file.is_open()) {
        _file << line;
        _file.flush();
    }

    if (roomId >= 0) {
        auto it = _roomFiles.find(roomId);
        if (it == _roomFiles.end()) {
            std::filesystem::path directory("logs");
            std::filesystem::path filePath = directory / ("room_" + std::to_string(roomId) + ".log");
            auto newFile                   = std::make_unique<std::ofstream>(filePath, std::ios::app);
            if (newFile->is_open()) {
                auto res = _roomFiles.emplace(roomId, std::move(newFile));
                it       = res.first;
            }
        }

        if (it != _roomFiles.end() && it->second && it->second->is_open()) {
            const std::string roomLine = "[" + timestamp + "][" + level + "] " + message + '\n';
            *(it->second) << roomLine;
            it->second->flush();
        }
    }

    bool tagAllowed = isTagEnabled(message);
    bool shouldLog  = alwaysConsole || (level == "ERROR") || (_verbose && tagAllowed);

    if (!shouldLog)
        return;

    if (_consoleEnabled) {
        if (level == "ERROR") {
            std::cerr << line;
        } else {
            std::cout << line;
        }
    }

    if (_postLogCallback) {
        _postLogCallback(line);
    }
}

void Logger::addTag(const std::string& tag)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string formattedTag = tag;
    if (!tag.empty() && tag[0] != '[') {
        formattedTag = "[" + tag + "]";
    }
    _enabledTags.insert(formattedTag);
    _tagFilterActive = !_enabledTags.empty();
}

void Logger::removeTag(const std::string& tag)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string formattedTag = tag;
    if (!tag.empty() && tag[0] != '[') {
        formattedTag = "[" + tag + "]";
    }
    _enabledTags.erase(formattedTag);
    _tagFilterActive = !_enabledTags.empty();
}

std::vector<std::string> Logger::getEnabledTags() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return std::vector<std::string>(_enabledTags.begin(), _enabledTags.end());
}

std::vector<std::string> Logger::getAllKnownTags() const
{
    return {"[Net]",     "[Packets]",     "[Game]",    "[Collision]",   "[Spawn]",
            "[Level]",   "[Replication]", "[Network]", "[Player]",      "[Death]",
            "[Respawn]", "[Snapshot]",    "[Input]",   "[LobbyServer]", "[InstanceManager]"};
}

bool Logger::hasTag(const std::string& tag) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string formattedTag = tag;
    if (!tag.empty() && tag[0] != '[') {
        formattedTag = "[" + tag + "]";
    }
    return _enabledTags.contains(formattedTag);
}
