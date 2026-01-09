#pragma once

#include <cstdint>
#include <string>

struct User
{
    std::uint32_t id;
    std::string username;
    std::string passwordHash;
    std::int64_t createdAt;
    std::int64_t lastLogin;
};

struct UserStats
{
    std::uint32_t userId;
    std::uint32_t gamesPlayed;
    std::uint32_t wins;
    std::uint32_t losses;
    std::uint64_t totalScore;
};

struct SessionToken
{
    std::uint32_t id;
    std::uint32_t userId;
    std::string tokenHash;
    std::int64_t expiresAt;
    std::int64_t createdAt;
};
