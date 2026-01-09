#include "auth/AuthService.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>

namespace
{

    std::string generateSalt()
    {
        unsigned char salt[16];
        if (RAND_bytes(salt, sizeof(salt)) != 1) {
            throw std::runtime_error("Failed to generate salt");
        }

        std::ostringstream oss;
        for (unsigned char byte : salt) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }

    std::string pbkdf2Hash(const std::string& password, const std::string& salt, int iterations)
    {
        unsigned char hash[32];

        if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.length()),
                              reinterpret_cast<const unsigned char*>(salt.c_str()), static_cast<int>(salt.length()),
                              iterations, EVP_sha256(), sizeof(hash), hash) != 1) {
            throw std::runtime_error("Failed to hash password");
        }

        std::ostringstream oss;
        for (unsigned char byte : hash) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return oss.str();
    }

} // namespace

AuthService::AuthService(const std::string& jwtSecret) : jwtSecret_(jwtSecret) {}

std::string AuthService::hashPassword(const std::string& password) const
{
    std::string salt       = generateSalt();
    int iterations         = 1 << BCRYPT_COST;
    std::string hashedPass = pbkdf2Hash(password, salt, iterations);

    return salt + ":" + hashedPass;
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash) const
{
    size_t colonPos = hash.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }

    std::string salt       = hash.substr(0, colonPos);
    std::string storedHash = hash.substr(colonPos + 1);

    int iterations           = 1 << BCRYPT_COST;
    std::string computedHash = pbkdf2Hash(password, salt, iterations);

    return computedHash == storedHash;
}

std::string AuthService::generateJWT(std::uint32_t userId, const std::string& username) const
{
    auto now    = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::hours(24 * JWT_EXPIRY_DAYS);

    auto token = jwt::create()
                     .set_issuer("rtype-server")
                     .set_type("JWT")
                     .set_issued_at(now)
                     .set_expires_at(expiry)
                     .set_payload_claim("userId", jwt::claim(std::to_string(userId)))
                     .set_payload_claim("username", jwt::claim(username))
                     .sign(jwt::algorithm::hs256{jwtSecret_});

    return token;
}

std::optional<JWTPayload> AuthService::validateJWT(const std::string& token) const
{
    try {
        auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{jwtSecret_}).with_issuer("rtype-server");

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        JWTPayload payload;
        payload.userId    = std::stoul(decoded.get_payload_claim("userId").as_string());
        payload.username  = decoded.get_payload_claim("username").as_string();
        payload.issuedAt  = std::chrono::system_clock::to_time_t(decoded.get_issued_at());
        payload.expiresAt = std::chrono::system_clock::to_time_t(decoded.get_expires_at());

        if (!payload.isValid()) {
            return std::nullopt;
        }

        return payload;
    } catch (const std::exception& e) {
        std::cerr << "JWT validation failed: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::string AuthService::hashToken(const std::string& token) const
{
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        throw std::runtime_error("Failed to create hash context");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(ctx, token.c_str(), token.length()) != 1 || EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to hash token");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (unsigned int i = 0; i < hashLen; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return oss.str();
}
