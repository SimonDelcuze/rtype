#pragma once

#include "network/LevelEventData.hpp"

#include <optional>
#include <vector>

class LevelEventParser
{
  public:
    static std::optional<LevelEventData> parse(const std::vector<std::uint8_t>& data);
};
