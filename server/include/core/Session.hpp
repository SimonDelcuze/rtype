#pragma once

#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

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

    bool authenticated = false;
    std::optional<std::uint32_t> userId;
    std::string username;
    std::string jwtToken;
};

std::string endpointKey(const IpEndpoint& ep);
