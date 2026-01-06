#include "network/LobbyPackets.hpp"
#include "network/PacketHeader.hpp"

#include <gtest/gtest.h>

class LobbyPacketsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        sequence_ = 42;
    }

    std::uint16_t sequence_;
};

TEST_F(LobbyPacketsTest, BuildListRoomsPacketHasValidHeader)
{
    auto packet = buildListRoomsPacket(sequence_);

    ASSERT_GE(packet.size(), PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerOpt = PacketHeader::decode(packet.data(), packet.size());
    ASSERT_TRUE(headerOpt.has_value());
    const auto& header = *headerOpt;

    EXPECT_EQ(header.packetType, static_cast<std::uint8_t>(PacketType::ClientToServer));
    EXPECT_EQ(header.messageType, static_cast<std::uint8_t>(MessageType::LobbyListRooms));
    EXPECT_EQ(header.sequenceId, sequence_);
    EXPECT_EQ(header.payloadSize, 0);
}

TEST_F(LobbyPacketsTest, BuildCreateRoomPacketHasValidHeader)
{
    auto packet = buildCreateRoomPacket(sequence_);

    ASSERT_GE(packet.size(), PacketHeader::kSize + PacketHeader::kCrcSize);

    auto headerOpt = PacketHeader::decode(packet.data(), packet.size());
    ASSERT_TRUE(headerOpt.has_value());
    const auto& header = *headerOpt;

    EXPECT_EQ(header.packetType, static_cast<std::uint8_t>(PacketType::ClientToServer));
    EXPECT_EQ(header.messageType, static_cast<std::uint8_t>(MessageType::LobbyCreateRoom));
    EXPECT_EQ(header.sequenceId, sequence_);
    EXPECT_EQ(header.payloadSize, 0);
}

TEST_F(LobbyPacketsTest, BuildJoinRoomPacketHasValidHeader)
{
    std::uint32_t roomId = 123;
    auto packet          = buildJoinRoomPacket(roomId, sequence_);

    ASSERT_GE(packet.size(), PacketHeader::kSize + 4 + PacketHeader::kCrcSize);

    auto headerOpt = PacketHeader::decode(packet.data(), packet.size());
    ASSERT_TRUE(headerOpt.has_value());
    const auto& header = *headerOpt;

    EXPECT_EQ(header.packetType, static_cast<std::uint8_t>(PacketType::ClientToServer));
    EXPECT_EQ(header.messageType, static_cast<std::uint8_t>(MessageType::LobbyJoinRoom));
    EXPECT_EQ(header.sequenceId, sequence_);
    EXPECT_EQ(header.payloadSize, 4);
}

TEST_F(LobbyPacketsTest, BuildJoinRoomPacketContainsCorrectRoomId)
{
    std::uint32_t expectedRoomId = 123;
    auto packet                  = buildJoinRoomPacket(expectedRoomId, sequence_);

    const std::uint8_t* payload = packet.data() + PacketHeader::kSize;
    std::uint32_t decodedRoomId =
        (static_cast<std::uint32_t>(payload[0]) << 24) | (static_cast<std::uint32_t>(payload[1]) << 16) |
        (static_cast<std::uint32_t>(payload[2]) << 8) | static_cast<std::uint32_t>(payload[3]);

    EXPECT_EQ(decodedRoomId, expectedRoomId);
}

TEST_F(LobbyPacketsTest, ParseRoomListPacketEmpty)
{
    std::vector<std::uint8_t> packet;

    PacketHeader header;
    header.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType  = static_cast<std::uint8_t>(MessageType::LobbyRoomList);
    header.sequenceId   = sequence_;
    header.payloadSize  = 2;
    header.originalSize = 2;

    auto headerBytes = header.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint16_t roomCount = 0;
    packet.push_back(static_cast<std::uint8_t>(roomCount >> 8));
    packet.push_back(static_cast<std::uint8_t>(roomCount));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    auto result = parseRoomListPacket(packet.data(), packet.size());

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->rooms.empty());
}

TEST_F(LobbyPacketsTest, ParseRoomListPacketSingleRoom)
{
    std::vector<std::uint8_t> packet;

    PacketHeader header;
    header.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType  = static_cast<std::uint8_t>(MessageType::LobbyRoomList);
    header.sequenceId   = sequence_;
    header.payloadSize  = 2 + 11;
    header.originalSize = 2 + 11;

    auto headerBytes = header.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint16_t roomCount = 1;
    packet.push_back(static_cast<std::uint8_t>(roomCount >> 8));
    packet.push_back(static_cast<std::uint8_t>(roomCount));

    std::uint32_t roomId      = 5;
    std::uint16_t playerCount = 2;
    std::uint16_t maxPlayers  = 4;
    std::uint16_t port        = 50105;
    std::uint8_t state        = static_cast<std::uint8_t>(RoomState::Waiting);

    packet.push_back(static_cast<std::uint8_t>(roomId >> 24));
    packet.push_back(static_cast<std::uint8_t>(roomId >> 16));
    packet.push_back(static_cast<std::uint8_t>(roomId >> 8));
    packet.push_back(static_cast<std::uint8_t>(roomId));

    packet.push_back(static_cast<std::uint8_t>(playerCount >> 8));
    packet.push_back(static_cast<std::uint8_t>(playerCount));

    packet.push_back(static_cast<std::uint8_t>(maxPlayers >> 8));
    packet.push_back(static_cast<std::uint8_t>(maxPlayers));

    packet.push_back(static_cast<std::uint8_t>(port >> 8));
    packet.push_back(static_cast<std::uint8_t>(port));

    packet.push_back(state);

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    auto result = parseRoomListPacket(packet.data(), packet.size());

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->rooms.size(), 1);

    const auto& room = result->rooms[0];
    EXPECT_EQ(room.roomId, roomId);
    EXPECT_EQ(room.playerCount, playerCount);
    EXPECT_EQ(room.maxPlayers, maxPlayers);
    EXPECT_EQ(room.state, RoomState::Waiting);
    EXPECT_EQ(room.port, port);
}

TEST_F(LobbyPacketsTest, ParseRoomCreatedPacket)
{
    std::vector<std::uint8_t> packet;

    PacketHeader header;
    header.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType  = static_cast<std::uint8_t>(MessageType::LobbyRoomCreated);
    header.sequenceId   = sequence_;
    header.payloadSize  = 6;
    header.originalSize = 6;

    auto headerBytes = header.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint32_t roomId = 7;
    std::uint16_t port   = 50107;

    packet.push_back(static_cast<std::uint8_t>(roomId >> 24));
    packet.push_back(static_cast<std::uint8_t>(roomId >> 16));
    packet.push_back(static_cast<std::uint8_t>(roomId >> 8));
    packet.push_back(static_cast<std::uint8_t>(roomId));

    packet.push_back(static_cast<std::uint8_t>(port >> 8));
    packet.push_back(static_cast<std::uint8_t>(port));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    auto result = parseRoomCreatedPacket(packet.data(), packet.size());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->roomId, roomId);
    EXPECT_EQ(result->port, port);
}

TEST_F(LobbyPacketsTest, ParseJoinSuccessPacket)
{
    std::vector<std::uint8_t> packet;

    PacketHeader header;
    header.packetType   = static_cast<std::uint8_t>(PacketType::ServerToClient);
    header.messageType  = static_cast<std::uint8_t>(MessageType::LobbyJoinSuccess);
    header.sequenceId   = sequence_;
    header.payloadSize  = 6;
    header.originalSize = 6;

    auto headerBytes = header.encode();
    packet.insert(packet.end(), headerBytes.begin(), headerBytes.end());

    std::uint32_t roomId = 3;
    std::uint16_t port   = 50103;

    packet.push_back(static_cast<std::uint8_t>(roomId >> 24));
    packet.push_back(static_cast<std::uint8_t>(roomId >> 16));
    packet.push_back(static_cast<std::uint8_t>(roomId >> 8));
    packet.push_back(static_cast<std::uint8_t>(roomId));

    packet.push_back(static_cast<std::uint8_t>(port >> 8));
    packet.push_back(static_cast<std::uint8_t>(port));

    std::uint32_t crc = PacketHeader::crc32(packet.data(), packet.size());
    packet.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    packet.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    auto result = parseJoinSuccessPacket(packet.data(), packet.size());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->roomId, roomId);
    EXPECT_EQ(result->port, port);
}

TEST_F(LobbyPacketsTest, ParseInvalidPacketTooSmall)
{
    std::vector<std::uint8_t> packet = {0x01, 0x02};

    auto result = parseRoomListPacket(packet.data(), packet.size());
    EXPECT_FALSE(result.has_value());
}
