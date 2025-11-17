#include "Main.hpp"
#include "Test.hpp"
#include <iostream>


void run_server()
{
    std::cout << "server\n";
    shared_hello("server");
}


int main()
{
    run_server();
    return 0;
}
