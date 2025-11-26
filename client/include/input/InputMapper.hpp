#pragma once

#include <cstdint>

class InputMapper
{
  public:
    static constexpr std::uint16_t UpFlag = 1u << 0;
    static constexpr std::uint16_t DownFlag = 1u << 1;
    static constexpr std::uint16_t LeftFlag = 1u << 2;
    static constexpr std::uint16_t RightFlag = 1u << 3;
    static constexpr std::uint16_t FireFlag = 1u << 4;

    virtual ~InputMapper() = default;
    virtual std::uint16_t pollFlags() const;
};
