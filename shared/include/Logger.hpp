#pragma once

#include <fstream>
#include <mutex>
#include <string>

class Logger
{
  public:
    static Logger& instance();

    void setVerbose(bool enabled);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

  private:
    Logger();
    ~Logger();

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const std::string& level, const std::string& message, bool alwaysConsole);

    std::ofstream _file;
    std::mutex _mutex;
    bool _verbose;
};
