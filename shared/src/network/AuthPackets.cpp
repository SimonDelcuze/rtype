#include "network/AuthPackets.hpp"

#include <cstring>

namespace
{

    constexpr std::size_t MAX_USERNAME_LENGTH = 32;
    constexpr std::size_t MAX_PASSWORD_LENGTH = 64;
    constexpr std::size_t MAX_TOKEN_LENGTH    = 512;

    void writeString(std::vector<std::uint8_t>& packet, const std::string& str, std::size_t maxLength)
    {
        std::uint16_t length = static_cast<std::uint16_t>(std::min(str.length(), maxLength));
        packet.push_back(static_cast<std::uint8_t>((length >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(length & 0xFF));

        for (std::size_t i = 0; i < length; ++i) {
            packet.push_back(static_cast<std::uint8_t>(str[i]));
        }
    }

    std::optional<std::string> readString(const std::uint8_t* data, std::size_t& offset, std::size_t totalSize,
                                          std::size_t maxLength)
    {
        if (offset + 2 > totalSize) {
            return std::nullopt;
        }

        std::uint16_t length = static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8) |
                                                          static_cast<std::uint16_t>(data[offset + 1]));
        offset += 2;

        if (length > maxLength || offset + length > totalSize) {
            return std::nullopt;
        }

        std::string str(reinterpret_cast<const char*>(data + offset), length);
        offset += length;
        return str;
    }

    void appendCrc(std::vector<std::uint8_t>& packet)
    {
        std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
        packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));
    }

} // namespace

// Login Request/Response

std::vector<std::uint8_t> buildLoginRequestPacket(const std::string& username, const std::string& password,
                                                  std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::AuthLoginRequest);
    hdr.sequenceId  = sequence;

    std::vector<std::uint8_t> packet;
    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::size_t payloadStart = packet.size();

    writeString(packet, username, MAX_USERNAME_LENGTH);
    writeString(packet, password, MAX_PASSWORD_LENGTH);

    hdr.payloadSize  = static_cast<std::uint16_t>(packet.size() - payloadStart);
    hdr.originalSize = hdr.payloadSize;

    headerBytes = hdr.encode();
    std::copy(headerBytes.begin(), headerBytes.end(), packet.begin());

    appendCrc(packet);
    return packet;
}

std::vector<std::uint8_t> buildLoginResponsePacket(bool success, std::uint32_t userId, const std::string& token,
                                                   AuthErrorCode errorCode, std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::AuthLoginResponse);
    hdr.sequenceId  = sequence;

    std::vector<std::uint8_t> packet;
    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::size_t payloadStart = packet.size();

    packet.push_back(success ? 0x01 : 0x00);

    packet.push_back(static_cast<std::uint8_t>((userId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((userId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((userId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(userId & 0xFF));

    writeString(packet, token, MAX_TOKEN_LENGTH);

    packet.push_back(static_cast<std::uint8_t>(errorCode));

    hdr.payloadSize  = static_cast<std::uint16_t>(packet.size() - payloadStart);
    hdr.originalSize = hdr.payloadSize;

    headerBytes = hdr.encode();
    std::copy(headerBytes.begin(), headerBytes.end(), packet.begin());

    appendCrc(packet);
    return packet;
}

std::optional<LoginRequestData> parseLoginRequestPacket(const std::uint8_t* data, std::size_t size)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;

    auto username = readString(data, offset, size, MAX_USERNAME_LENGTH);
    if (!username.has_value()) {
        return std::nullopt;
    }

    auto password = readString(data, offset, size, MAX_PASSWORD_LENGTH);
    if (!password.has_value()) {
        return std::nullopt;
    }

    LoginRequestData result;
    result.username = username.value();
    result.password = password.value();
    return result;
}

std::optional<LoginResponseData> parseLoginResponsePacket(const std::uint8_t* data, std::size_t size)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;

    if (offset + 1 + 4 > size) {
        return std::nullopt;
    }

    LoginResponseData result;
    result.success = data[offset] != 0x00;
    offset++;

    result.userId = (static_cast<std::uint32_t>(data[offset]) << 24) |
                    (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
                    (static_cast<std::uint32_t>(data[offset + 2]) << 8) | static_cast<std::uint32_t>(data[offset + 3]);
    offset += 4;

    auto token = readString(data, offset, size, MAX_TOKEN_LENGTH);
    if (!token.has_value()) {
        return std::nullopt;
    }
    result.token = token.value();

    if (offset + 1 > size) {
        return std::nullopt;
    }
    result.errorCode = static_cast<AuthErrorCode>(data[offset]);

    return result;
}

// Register Request/Response

std::vector<std::uint8_t> buildRegisterRequestPacket(const std::string& username, const std::string& password,
                                                     std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::AuthRegisterRequest);
    hdr.sequenceId  = sequence;

    std::vector<std::uint8_t> packet;
    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::size_t payloadStart = packet.size();

    writeString(packet, username, MAX_USERNAME_LENGTH);
    writeString(packet, password, MAX_PASSWORD_LENGTH);

    hdr.payloadSize  = static_cast<std::uint16_t>(packet.size() - payloadStart);
    hdr.originalSize = hdr.payloadSize;

    headerBytes = hdr.encode();
    std::copy(headerBytes.begin(), headerBytes.end(), packet.begin());

    appendCrc(packet);
    return packet;
}

std::vector<std::uint8_t> buildRegisterResponsePacket(bool success, std::uint32_t userId, AuthErrorCode errorCode,
                                                      std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::AuthRegisterResponse);
    hdr.sequenceId  = sequence;

    constexpr std::uint16_t payloadSize = 1 + 4 + 1;
    hdr.payloadSize                     = payloadSize;
    hdr.originalSize                    = payloadSize;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + payloadSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(success ? 0x01 : 0x00);

    packet.push_back(static_cast<std::uint8_t>((userId >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((userId >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((userId >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(userId & 0xFF));

    packet.push_back(static_cast<std::uint8_t>(errorCode));

    appendCrc(packet);
    return packet;
}

std::optional<RegisterRequestData> parseRegisterRequestPacket(const std::uint8_t* data, std::size_t size)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;

    auto username = readString(data, offset, size, MAX_USERNAME_LENGTH);
    if (!username.has_value()) {
        return std::nullopt;
    }

    auto password = readString(data, offset, size, MAX_PASSWORD_LENGTH);
    if (!password.has_value()) {
        return std::nullopt;
    }

    RegisterRequestData result;
    result.username = username.value();
    result.password = password.value();
    return result;
}

std::optional<RegisterResponseData> parseRegisterResponsePacket(const std::uint8_t* data, std::size_t size)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;

    if (offset + 1 + 4 + 1 > size) {
        return std::nullopt;
    }

    RegisterResponseData result;
    result.success = data[offset] != 0x00;
    offset++;

    result.userId = (static_cast<std::uint32_t>(data[offset]) << 24) |
                    (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
                    (static_cast<std::uint32_t>(data[offset + 2]) << 8) | static_cast<std::uint32_t>(data[offset + 3]);
    offset += 4;

    result.errorCode = static_cast<AuthErrorCode>(data[offset]);

    return result;
}

// Change Password Request/Response

std::vector<std::uint8_t> buildChangePasswordRequestPacket(const std::string& oldPassword,
                                                           const std::string& newPassword, const std::string& token,
                                                           std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::AuthChangePasswordRequest);
    hdr.sequenceId  = sequence;

    std::vector<std::uint8_t> packet;
    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::size_t payloadStart = packet.size();

    writeString(packet, oldPassword, MAX_PASSWORD_LENGTH);
    writeString(packet, newPassword, MAX_PASSWORD_LENGTH);
    writeString(packet, token, MAX_TOKEN_LENGTH);

    hdr.payloadSize  = static_cast<std::uint16_t>(packet.size() - payloadStart);
    hdr.originalSize = hdr.payloadSize;

    headerBytes = hdr.encode();
    std::copy(headerBytes.begin(), headerBytes.end(), packet.begin());

    appendCrc(packet);
    return packet;
}

std::vector<std::uint8_t> buildChangePasswordResponsePacket(bool success, AuthErrorCode errorCode,
                                                            std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::AuthChangePasswordResponse);
    hdr.sequenceId   = sequence;
    hdr.payloadSize  = 2;
    hdr.originalSize = 2;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + 2 + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    packet.push_back(success ? 0x01 : 0x00);
    packet.push_back(static_cast<std::uint8_t>(errorCode));

    appendCrc(packet);
    return packet;
}

std::optional<ChangePasswordRequestData> parseChangePasswordRequestPacket(const std::uint8_t* data, std::size_t size)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;

    auto oldPassword = readString(data, offset, size, MAX_PASSWORD_LENGTH);
    if (!oldPassword.has_value()) {
        return std::nullopt;
    }

    auto newPassword = readString(data, offset, size, MAX_PASSWORD_LENGTH);
    if (!newPassword.has_value()) {
        return std::nullopt;
    }

    auto token = readString(data, offset, size, MAX_TOKEN_LENGTH);
    if (!token.has_value()) {
        return std::nullopt;
    }

    ChangePasswordRequestData result;
    result.oldPassword = oldPassword.value();
    result.newPassword = newPassword.value();
    result.token       = token.value();
    return result;
}

std::optional<ChangePasswordResponseData> parseChangePasswordResponsePacket(const std::uint8_t* data, std::size_t size)
{
    auto hdr = PacketHeader::decode(data, size);
    if (!hdr.has_value()) {
        return std::nullopt;
    }

    std::size_t offset = PacketHeader::kSize;

    if (offset + 2 > size) {
        return std::nullopt;
    }

    ChangePasswordResponseData result;
    result.success   = data[offset] != 0x00;
    result.errorCode = static_cast<AuthErrorCode>(data[offset + 1]);

    return result;
}

// Auth Required

std::vector<std::uint8_t> buildAuthRequiredPacket(std::uint16_t sequence)
{
    PacketHeader hdr;
    hdr.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType  = static_cast<std::uint8_t>(MessageType::AuthRequired);
    hdr.sequenceId   = sequence;
    hdr.payloadSize  = 0;
    hdr.originalSize = 0;

    std::vector<std::uint8_t> packet;
    packet.reserve(PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerBytes = hdr.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    appendCrc(packet);
    return packet;
}
