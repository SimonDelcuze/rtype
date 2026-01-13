#pragma once

#include <cstdint>
#include <string>

struct AuthResult
{
    bool authenticated;
    std::string username;
    std::string token;
    std::uint32_t userId;
    std::string password;
};
