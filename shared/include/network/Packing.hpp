#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Packing
{
    inline std::int16_t quantizeTo16(float value, float scale)
    {
        return static_cast<std::int16_t>(std::clamp(static_cast<int32_t>(std::round(value * scale)), -32768, 32767));
    }

    inline float dequantizeFrom16(std::int16_t value, float scale)
    {
        return static_cast<float>(value) / scale;
    }

    inline std::int8_t quantizeTo8(float value, float scale)
    {
        return static_cast<std::int8_t>(std::clamp(static_cast<int>(std::round(value * scale)), -128, 127));
    }

    inline float dequantizeFrom8(std::int8_t value, float scale)
    {
        return static_cast<float>(value) / scale;
    }

    inline std::uint8_t pack44(std::uint8_t high, std::uint8_t low)
    {
        return (static_cast<std::uint8_t>(high & 0x0F) << 4) | (static_cast<std::uint8_t>(low & 0x0F));
    }

    inline void unpack44(std::uint8_t packed, std::uint8_t& high, std::uint8_t& low)
    {
        high = (packed >> 4) & 0x0F;
        low  = packed & 0x0F;
    }
} // namespace Packing
