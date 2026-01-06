#include "network/NetworkCompression.hpp"

#include "errors/CompressionError.hpp"

#include <lz4.h>

namespace Compression
{
    std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& input)
    {
        if (input.empty())
            return {};

        int maxCompressedSize = LZ4_compressBound(static_cast<int>(input.size()));
        std::vector<std::uint8_t> output(maxCompressedSize);

        int compressedSize =
            LZ4_compress_default(reinterpret_cast<const char*>(input.data()), reinterpret_cast<char*>(output.data()),
                                 static_cast<int>(input.size()), maxCompressedSize);

        if (compressedSize <= 0)
            throw CompressionError("LZ4 compression failed");

        output.resize(compressedSize);
        return output;
    }

    std::vector<std::uint8_t> decompress(const std::uint8_t* data, std::size_t compressedLen, std::size_t originalSize)
    {
        if (compressedLen == 0 || originalSize == 0)
            return {};

        std::vector<std::uint8_t> output(originalSize);

        int decompressedSize =
            LZ4_decompress_safe(reinterpret_cast<const char*>(data), reinterpret_cast<char*>(output.data()),
                                static_cast<int>(compressedLen), static_cast<int>(originalSize));

        if (decompressedSize < 0 || static_cast<std::size_t>(decompressedSize) != originalSize)
            throw DecompressionError("LZ4 decompression failed or size mismatch");

        return output;
    }
} // namespace Compression
