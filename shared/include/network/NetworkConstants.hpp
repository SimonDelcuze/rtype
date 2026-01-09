#pragma once

#include "network/PacketHeader.hpp"

#include <cstddef>

namespace Network
{
    constexpr std::size_t kEthernetMTU = 1500;
    constexpr std::size_t kIPv4HeaderSize = 20;
    constexpr std::size_t kUDPHeaderSize = 8;
    constexpr std::size_t kMaxUDPPayload = kEthernetMTU - kIPv4HeaderSize - kUDPHeaderSize;
    constexpr std::size_t kMaxSafePacketPayload = kMaxUDPPayload - 20 - PacketHeader::kSize - PacketHeader::kCrcSize;

} // namespace Network
