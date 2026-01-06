#include "network/Packing.hpp"

#include <gtest/gtest.h>

TEST(Packing, QuantizeDeQuantize16)
{
    float original         = 1234.56F;
    float scale            = 10.0F;
    std::int16_t quantized = Packing::quantizeTo16(original, scale);
    float dequantized      = Packing::dequantizeFrom16(quantized, scale);

    EXPECT_EQ(quantized, 12346); // Rounded
    EXPECT_NEAR(dequantized, 1234.6F, 0.01F);
}

TEST(Packing, QuantizeOverflow)
{
    float huge             = 1000000.0F;
    float scale            = 10.0F;
    std::int16_t quantized = Packing::quantizeTo16(huge, scale);
    EXPECT_EQ(quantized, 32767);

    float tiny = -1000000.0F;
    quantized  = Packing::quantizeTo16(tiny, scale);
    EXPECT_EQ(quantized, -32768);
}

TEST(Packing, QuantizeDeQuantize8)
{
    float original        = 0.5F;
    float scale           = 100.0F;
    std::int8_t quantized = Packing::quantizeTo8(original, scale);
    float dequantized     = Packing::dequantizeFrom8(quantized, scale);

    EXPECT_EQ(quantized, 50);
    EXPECT_NEAR(dequantized, 0.5F, 0.01F);
}

TEST(Packing, PackUnpack44)
{
    std::uint8_t high   = 0x0A;
    std::uint8_t low    = 0x05;
    std::uint8_t packed = Packing::pack44(high, low);

    EXPECT_EQ(packed, 0xA5);

    std::uint8_t uHigh, uLow;
    Packing::unpack44(packed, uHigh, uLow);
    EXPECT_EQ(uHigh, 0x0A);
    EXPECT_EQ(uLow, 0x05);
}

TEST(Packing, Pack44Truncates)
{
    std::uint8_t high   = 0xFF; // Should be truncated to 0x0F
    std::uint8_t low    = 0x11; // Should be truncated to 0x01
    std::uint8_t packed = Packing::pack44(high, low);

    EXPECT_EQ(packed, 0xF1);
}
