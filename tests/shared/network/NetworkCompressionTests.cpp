#include "errors/CompressionError.hpp"
#include "network/NetworkCompression.hpp"

#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(NetworkCompression, RoundTrip)
{
    std::string text =
        "This is a test string that should be compressible by LZ4 because it has some repeating patterns. "
        "Repeating patterns. Repeating patterns. Repeating patterns. Repeating patterns.";
    std::vector<std::uint8_t> input(text.begin(), text.end());

    auto compressed = Compression::compress(input);
    EXPECT_LT(compressed.size(), input.size());

    auto decompressed = Compression::decompress(compressed.data(), compressed.size(), input.size());
    EXPECT_EQ(input, decompressed);
}

TEST(NetworkCompression, EmptyData)
{
    std::vector<std::uint8_t> input;
    auto compressed = Compression::compress(input);
    EXPECT_TRUE(compressed.empty());

    auto decompressed = Compression::decompress(nullptr, 0, 0);
    EXPECT_TRUE(decompressed.empty());
}

TEST(NetworkCompression, UncompressibleData)
{
    std::vector<std::uint8_t> input = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78};

    auto compressed = Compression::compress(input);

    auto decompressed = Compression::decompress(compressed.data(), compressed.size(), input.size());
    EXPECT_EQ(input, decompressed);
}

TEST(NetworkCompression, DecompressionFailure)
{
    std::vector<std::uint8_t> input = {1, 2, 3, 4, 5};
    auto compressed                 = Compression::compress(input);

    if (!compressed.empty())
        compressed[0] = ~compressed[0];

    EXPECT_THROW({ Compression::decompress(compressed.data(), compressed.size(), input.size()); }, DecompressionError);
}

TEST(NetworkCompression, LargeBuffer)
{
    std::vector<std::uint8_t> input(65536);
    for (size_t i = 0; i < input.size(); ++i) {
        input[i] = static_cast<std::uint8_t>(i % 256);
    }

    auto compressed = Compression::compress(input);
    EXPECT_LT(compressed.size(), input.size());

    auto decompressed = Compression::decompress(compressed.data(), compressed.size(), input.size());
    EXPECT_EQ(input, decompressed);
}

TEST(NetworkCompression, HighlyRedundant)
{
    std::vector<std::uint8_t> input(1000, 0x42);
    auto compressed = Compression::compress(input);
    EXPECT_LT(compressed.size(), 50);

    auto decompressed = Compression::decompress(compressed.data(), compressed.size(), input.size());
    EXPECT_EQ(input, decompressed);
}

TEST(NetworkCompression, PartialDecompressionSizeMismatch)
{
    std::vector<std::uint8_t> input = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    auto compressed                 = Compression::compress(input);

    EXPECT_THROW(
        { Compression::decompress(compressed.data(), compressed.size(), input.size() - 1); }, DecompressionError);

    EXPECT_THROW(
        { Compression::decompress(compressed.data(), compressed.size(), input.size() + 1); }, DecompressionError);
}
