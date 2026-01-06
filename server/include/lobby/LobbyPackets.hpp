#pragma once

#include "lobby/LobbyManager.hpp"
#include "network/PacketHeader.hpp"

#include <cstdint>
#include <vector>

std::vector<std::uint8_t> buildRoomListPacket(const std::vector<RoomInfo>& rooms, std::uint16_t sequence);

std::vector<std::uint8_t> buildRoomCreatedPacket(std::uint32_t roomId, std::uint16_t port, std::uint16_t sequence);

std::vector<std::uint8_t> buildJoinSuccessPacket(std::uint32_t roomId, std::uint16_t port, std::uint16_t sequence);

std::vector<std::uint8_t> buildJoinFailedPacket(std::uint16_t sequence);
