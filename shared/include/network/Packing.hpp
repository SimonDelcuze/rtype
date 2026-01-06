#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Packing
{
    /**
     * @brief Quantize a float value to an integer using a scale factor.
     *
     * @param value The float value to quantize.
     * @param scale The scale factor (e.g., 100.0 for 2 decimal places).
     * @return std::int16_t The quantized value.
     */
    inline std::int16_t quantizeTo16(float value, float scale)
    {
        return static_cast<std::int16_t>(std::clamp(static_cast<int32_t>(std::round(value * scale)), -32768, 32767));
    }

    /**
     * @brief De-quantize an integer value back to float.
     *
     * @param value The quantized integer value.
     * @param scale The inverse of the scale factor used during quantization.
     * @return float The de-quantized value.
     */
    inline float dequantizeFrom16(std::int16_t value, float scale)
    {
        return static_cast<float>(value) / scale;
    }

    /**
     * @brief Quantize a float value to an 8-bit integer.
     * useful for values in range [-1, 1] or [0, 1] with less precision.
     */
    inline std::int8_t quantizeTo8(float value, float scale)
    {
        return static_cast<std::int8_t>(std::clamp(static_cast<int>(std::round(value * scale)), -128, 127));
    }

    inline float dequantizeFrom8(std::int8_t value, float scale)
    {
        return static_cast<float>(value) / scale;
    }

    /**
     * @brief Pack two 4-bit values into a single byte.
     */
    inline std::uint8_t pack44(std::uint8_t high, std::uint8_t low)
    {
        return (static_cast<std::uint8_t>(high & 0x0F) << 4) | (static_cast<std::uint8_t>(low & 0x0F));
    }

    /**
     * @brief Unpack two 4-bit values from a single byte.
     */
    inline void unpack44(std::uint8_t packed, std::uint8_t& high, std::uint8_t& low)
    {
        high = (packed >> 4) & 0x0F;
        low  = packed & 0x0F;
    }
} // namespace Packing
