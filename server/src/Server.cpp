#include "Logger.hpp"
#include "Main.hpp"
#include "Test.hpp"

#include <iostream>

void run_server(void)
{
    std::cout << "server\n";
    shared_hello("server");
    Logger::instance().info("Server started");
}
