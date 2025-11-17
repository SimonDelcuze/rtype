#include "Main.hpp"
#include "Test.hpp"
#include <iostream>

void run_client()
{
    std::cout << "client\n";
    shared_hello("client");
}

int main()
{
    run_client();
    return 0;
}
