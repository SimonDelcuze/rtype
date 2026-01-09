#pragma once

#include <optional>
#include <string>

struct ClientOptions
{
    bool verbose    = false;
    bool useDefault = false;
    std::optional<std::string> serverIp;
    std::optional<int> serverPort;
};

ClientOptions parseOptions(int argc, char* argv[]);
void configureLogger(bool verbose);
