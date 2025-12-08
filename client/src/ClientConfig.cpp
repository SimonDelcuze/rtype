#include "ClientConfig.hpp"

#include "Logger.hpp"

ClientOptions parseOptions(int argc, char* argv[])
{
    ClientOptions options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v")
            options.verbose = true;
        else if (arg == "--default" || arg == "-d")
            options.useDefault = true;
    }
    return options;
}

void configureLogger(bool verbose)
{
    Logger::instance().setVerbose(verbose);
    Logger::instance().info("R-Type Client starting" + std::string(verbose ? " (verbose mode)" : ""));
}
