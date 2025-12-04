#pragma once

#include "network/LevelInitData.hpp"
#include "network/PacketHeader.hpp"

#include <cstdint>
#include <optional>
#include <vector>

class LevelInitParser
{
  public:
    static std::optional<LevelInitData> parse(const std::vector<std::uint8_t>& data);

  private:
    static bool validateHeader(const std::vector<std::uint8_t>& data);
    static bool ensureAvailable(std::size_t offset, std::size_t need, std::size_t total);
    static std::uint8_t readU8(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static std::uint16_t readU16(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static std::uint32_t readU32(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static std::string readString(const std::vector<std::uint8_t>& buf, std::size_t& offset);
    static std::optional<ArchetypeEntry> parseArchetype(const std::vector<std::uint8_t>& buf,
                                                        std::size_t& offset,
                                                        std::size_t total);
};
