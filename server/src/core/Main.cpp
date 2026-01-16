#include "Main.hpp"

#include "Logger.hpp"

#ifdef _WIN32
#include <timeapi.h>
#include <windows.h>
#endif

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
