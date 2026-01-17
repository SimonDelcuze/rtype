#include "Main.hpp"

#include "Logger.hpp"

// clang-format off
#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif
// clang-format on

#include <string>

int main(int argc, char* argv[])
{
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    (void) argc;
    (void) argv;
    Logger::instance().setVerbose(true);
    Logger::instance().loadTagConfig("server.log.config");
    run_server();
    return 0;
}
