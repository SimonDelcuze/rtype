#pragma once

#include "security/JWTPayload.hpp"

#include <optional>
#include <string>

class AuthService
{
  public:
    explicit AuthService(const std::string& jwtSecret);

    std::string hashPassword(const std::string& password) const;
    bool verifyPassword(const std::string& password, const std::string& hash) const;

    std::string generateJWT(std::uint32_t userId, const std::string& username) const;
    std::optional<JWTPayload> validateJWT(const std::string& token) const;

    std::string hashToken(const std::string& token) const;

  private:
    std::string jwtSecret_;
    static constexpr int BCRYPT_COST     = 12;
    static constexpr int JWT_EXPIRY_DAYS = 7;
};
