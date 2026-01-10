#pragma once

#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

struct ClientSession
{
    bool hello     = false;
    bool join      = false;
    bool ready     = false;
    bool started   = false;
    bool levelSent = false;
    std::uint32_t playerId{0};
    std::uint32_t roomId{0};
    IpEndpoint endpoint{};
    PlayerRole role{PlayerRole::Player};
    std::string playerName;

    bool authenticated = false;
    std::optional<std::uint32_t> userId;
    std::string username;
    std::string jwtToken;

    std::chrono::steady_clock::time_point lastActivity{std::chrono::steady_clock::now()};
};

std::string endpointKey(const IpEndpoint& ep);
