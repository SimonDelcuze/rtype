#include "Main.hpp"

#include "Logger.hpp"

#include <string>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    Logger::instance().setVerbose(true);
    Logger::instance().loadTagConfig("server.log.config");
    run_server();
    return 0;
}
