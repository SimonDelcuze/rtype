#pragma once

#include "network/UdpSocket.hpp"

#include <chrono>
#include <cstdint>

struct ClientTimeoutEvent
{
    IpEndpoint endpoint{};
    std::uint16_t lastSequenceId = 0;
    std::chrono::steady_clock::time_point lastPacketTime{};
};
