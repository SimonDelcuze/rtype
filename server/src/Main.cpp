#include "Main.hpp"

#include "Logger.hpp"

#include <string>

int main(int argc, char* argv[])
{
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        const std::string argument(argv[i]);
        if (argument == "--verbose" || argument == "-v") {
            verbose = true;
        }
    }

    Logger::instance().setVerbose(verbose);

    if (verbose) {
        Logger::instance().info("Verbose logging enabled");
    }

    run_server();
    return 0;
}
