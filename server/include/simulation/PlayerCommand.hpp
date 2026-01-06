#pragma once

#include <cstdint>

/**
 * @brief Pure gameplay command abstraction, decoupled from network protocol.
 *
 * This structure represents a player's input in a network-agnostic way.
 * The simulation layer only deals with PlayerCommand, never with ReceivedInput or network packets.
 */
struct PlayerCommand
{
    std::uint32_t playerId;
    std::uint16_t inputFlags;
    float x;
    float y;
    float angle;
    std::uint16_t sequenceId;
    std::uint32_t tickId;
};
