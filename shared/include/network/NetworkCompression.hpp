#pragma once

#include <cstdint>
#include <vector>

namespace Compression
{
    std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& input);

    std::vector<std::uint8_t> decompress(const std::uint8_t* data, std::size_t compressedLen, std::size_t originalSize);
} // namespace Compression
