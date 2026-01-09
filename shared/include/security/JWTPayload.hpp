#pragma once

#include <chrono>
#include <cstdint>
#include <string>

struct JWTPayload
{
    std::uint32_t userId;
    std::string username;
    std::int64_t issuedAt;
    std::int64_t expiresAt;

    bool isExpired() const
    {
        auto now          = std::chrono::system_clock::now();
        auto nowTimestamp = std::chrono::system_clock::to_time_t(now);
        return nowTimestamp >= expiresAt;
    }

    bool isValid() const
    {
        return !isExpired() && userId > 0 && !username.empty();
    }
};
