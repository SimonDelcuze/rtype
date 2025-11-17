#include "Main.hpp"
#include "Test.hpp"
#include <iostream>

void run_client(void)
{
    std::cout << "client\n";
    shared_hello("client");
}
