#include "Test.hpp"
#include <iostream>

void shared_hello(const std::string& from) {
    std::cout << "[shared] hello from " << from << "\n";
}
