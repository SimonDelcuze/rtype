#pragma once

#include "network/PacketHeader.hpp"

#include <cstddef>

namespace Network
{
    /**
     * @brief Standard Ethernet MTU size.
     * 1500 bytes is the standard for most routers and networks.
     */
    constexpr std::size_t kEthernetMTU = 1500;

    /**
     * @brief Typical IPv4 header size without options.
     */
    constexpr std::size_t kIPv4HeaderSize = 20;

    /**
     * @brief Standard UDP header size.
     */
    constexpr std::size_t kUDPHeaderSize = 8;

    /**
     * @brief Maximum theoretical UDP payload that fits in a single Ethernet frame.
     * 1500 - 20 (IP) - 8 (UDP) = 1472 bytes.
     */
    constexpr std::size_t kMaxUDPPayload = kEthernetMTU - kIPv4HeaderSize - kUDPHeaderSize;

    /**
     * @brief Maximum safe payload size for our game packets.
     * We leave some extra room (e.g., 20 bytes) for safety against PPPoE/VPN overhead
     * and subtract our own PacketHeader and CRC size.
     * 1472 - 20 (safety) - 17 (header) - 4 (crc) = 1431 bytes.
     */
    constexpr std::size_t kMaxSafePacketPayload = kMaxUDPPayload - 20 - PacketHeader::kSize - PacketHeader::kCrcSize;

} // namespace Network
