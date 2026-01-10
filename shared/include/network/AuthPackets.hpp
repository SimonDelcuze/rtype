#pragma once

#include "network/PacketHeader.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class AuthErrorCode : std::uint8_t
{
    Success            = 0x00,
    InvalidCredentials = 0x01,
    UsernameTaken      = 0x02,
    WeakPassword       = 0x03,
    InvalidToken       = 0x04,
    ServerError        = 0x05,
    Unauthorized       = 0x06,
    AlreadyConnected   = 0x07
};

struct LoginRequestData
{
    std::string username;
    std::string password;
};

struct LoginResponseData
{
    bool success;
    std::uint32_t userId;
    std::string token;
    AuthErrorCode errorCode;
};

struct RegisterRequestData
{
    std::string username;
    std::string password;
};

struct RegisterResponseData
{
    bool success;
    std::uint32_t userId;
    AuthErrorCode errorCode;
};

struct ChangePasswordRequestData
{
    std::string oldPassword;
    std::string newPassword;
    std::string token;
};

struct ChangePasswordResponseData
{
    bool success;
    AuthErrorCode errorCode;
};

std::vector<std::uint8_t> buildLoginRequestPacket(const std::string& username, const std::string& password,
                                                  std::uint16_t sequence);

std::vector<std::uint8_t> buildLoginResponsePacket(bool success, std::uint32_t userId, const std::string& token,
                                                   AuthErrorCode errorCode, std::uint16_t sequence);

std::vector<std::uint8_t> buildRegisterRequestPacket(const std::string& username, const std::string& password,
                                                     std::uint16_t sequence);

std::vector<std::uint8_t> buildRegisterResponsePacket(bool success, std::uint32_t userId, AuthErrorCode errorCode,
                                                      std::uint16_t sequence);

std::vector<std::uint8_t> buildChangePasswordRequestPacket(const std::string& oldPassword,
                                                           const std::string& newPassword, const std::string& token,
                                                           std::uint16_t sequence);

std::vector<std::uint8_t> buildChangePasswordResponsePacket(bool success, AuthErrorCode errorCode,
                                                            std::uint16_t sequence);

std::vector<std::uint8_t> buildAuthRequiredPacket(std::uint16_t sequence);

std::optional<LoginRequestData> parseLoginRequestPacket(const std::uint8_t* data, std::size_t size);
std::optional<LoginResponseData> parseLoginResponsePacket(const std::uint8_t* data, std::size_t size);
std::optional<RegisterRequestData> parseRegisterRequestPacket(const std::uint8_t* data, std::size_t size);
std::optional<RegisterResponseData> parseRegisterResponsePacket(const std::uint8_t* data, std::size_t size);
std::optional<ChangePasswordRequestData> parseChangePasswordRequestPacket(const std::uint8_t* data, std::size_t size);
std::optional<ChangePasswordResponseData> parseChangePasswordResponsePacket(const std::uint8_t* data, std::size_t size);
