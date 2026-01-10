#include "lobby/PasswordUtils.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>

namespace
{
    constexpr std::array<std::uint32_t, 64> kSHA256_K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    std::uint32_t rotr(std::uint32_t x, std::uint32_t n)
    {
        return (x >> n) | (x << (32 - n));
    }

    void sha256Transform(std::array<std::uint32_t, 8>& state, const std::uint8_t* block)
    {
        std::array<std::uint32_t, 64> w{};

        for (std::size_t i = 0; i < 16; i++) {
            w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
                   (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) | static_cast<std::uint32_t>(block[i * 4 + 3]);
        }

        for (std::size_t i = 16; i < 64; i++) {
            std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i]             = w[i - 16] + s0 + w[i - 7] + s1;
        }

        auto a = state[0];
        auto b = state[1];
        auto c = state[2];
        auto d = state[3];
        auto e = state[4];
        auto f = state[5];
        auto g = state[6];
        auto h = state[7];

        for (std::size_t i = 0; i < 64; i++) {
            std::uint32_t S1    = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            std::uint32_t ch    = (e & f) ^ ((~e) & g);
            std::uint32_t temp1 = h + S1 + ch + kSHA256_K[i] + w[i];
            std::uint32_t S0    = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            std::uint32_t maj   = (a & b) ^ (a & c) ^ (b & c);
            std::uint32_t temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    std::string sha256(const std::string& input)
    {
        std::array<std::uint32_t, 8> state = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                              0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

        std::vector<std::uint8_t> data(input.begin(), input.end());
        std::uint64_t bitlen = data.size() * 8;

        data.push_back(0x80);
        while ((data.size() % 64) != 56) {
            data.push_back(0x00);
        }

        for (int i = 7; i >= 0; i--) {
            data.push_back(static_cast<std::uint8_t>((bitlen >> (i * 8)) & 0xff));
        }

        for (std::size_t i = 0; i < data.size(); i += 64) {
            sha256Transform(state, &data[i]);
        }

        std::ostringstream result;
        for (const auto& val : state) {
            result << std::hex << std::setfill('0') << std::setw(8) << val;
        }

        return result.str();
    }
} // namespace

namespace PasswordUtils
{
    std::string hashPassword(const std::string& password)
    {
        return sha256(password);
    }

    bool verifyPassword(const std::string& password, const std::string& hash)
    {
        return hashPassword(password) == hash;
    }

    std::string generateInviteCode()
    {
        static const char chars[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<std::size_t> dis(0, sizeof(chars) - 2);

        std::string code;
        for (int i = 0; i < 6; i++) {
            code += chars[dis(gen)];
        }
        return code;
    }
} // namespace PasswordUtils
