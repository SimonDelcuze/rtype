#pragma once

#include "network/PacketHeader.hpp"
#include "network/UdpSocket.hpp"

#include <cstdint>
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
};

std::string endpointKey(const IpEndpoint& ep);
