#include "concurrency/ThreadSafeQueue.hpp"
#include "network/NetworkMessageHandler.hpp"
#include "network/PacketHeader.hpp"

#include <atomic>
#include <gtest/gtest.h>
#include <vector>

class ConnectionFlowTest : public ::testing::Test
{
  protected:
    ThreadSafeQueue<std::vector<std::uint8_t>> rawQueue;
    ThreadSafeQueue<SnapshotParseResult> snapshotQueue;
    ThreadSafeQueue<LevelInitData> levelInitQueue;
    ThreadSafeQueue<LevelEventData> levelEventQueue;
    ThreadSafeQueue<EntitySpawnPacket> spawnQueue;
    ThreadSafeQueue<EntityDestroyedPacket> destroyQueue;

    std::atomic<bool> handshakeFlag{false};
    std::atomic<bool> allReadyFlag{false};
    std::atomic<int> countdownValue{-1};
    std::atomic<bool> gameStartFlag{false};
    std::atomic<bool> joinDeniedFlag{false};
    std::atomic<bool> joinAcceptedFlag{false};

    std::unique_ptr<NetworkMessageHandler> handler;

    void SetUp() override
    {
        handler = std::make_unique<NetworkMessageHandler>(
            rawQueue, snapshotQueue, levelInitQueue, levelEventQueue, spawnQueue, destroyQueue, nullptr, nullptr,
            &handshakeFlag, &allReadyFlag, &countdownValue, &gameStartFlag, &joinDeniedFlag, &joinAcceptedFlag);
    }

    std::vector<std::uint8_t> createPacket(MessageType type, std::uint16_t payloadSize = 0)
    {
        PacketHeader hdr{};
        hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
        hdr.messageType = static_cast<std::uint8_t>(type);
        hdr.payloadSize = payloadSize;

        auto encoded = hdr.encode();
        std::vector<std::uint8_t> pkt(encoded.begin(), encoded.end());

        for (std::uint16_t i = 0; i < payloadSize; ++i) {
            pkt.push_back(0);
        }

        auto crc = PacketHeader::crc32(pkt.data(), pkt.size());
        pkt.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
        pkt.push_back(static_cast<std::uint8_t>(crc & 0xFF));

        return pkt;
    }
};

TEST_F(ConnectionFlowTest, HandleJoinAccept)
{
    auto pkt = createPacket(MessageType::ServerJoinAccept);
    rawQueue.push(pkt);

    handler->poll();

    EXPECT_TRUE(joinAcceptedFlag.load());
    EXPECT_FALSE(joinDeniedFlag.load());
}

TEST_F(ConnectionFlowTest, HandleJoinDeny)
{
    auto pkt = createPacket(MessageType::ServerJoinDeny);
    rawQueue.push(pkt);

    handler->poll();

    EXPECT_FALSE(joinAcceptedFlag.load());
    EXPECT_TRUE(joinDeniedFlag.load());
}

TEST_F(ConnectionFlowTest, HandleGameStart)
{
    auto pkt = createPacket(MessageType::GameStart);
    rawQueue.push(pkt);

    handler->poll();

    EXPECT_TRUE(gameStartFlag.load());
    EXPECT_TRUE(handshakeFlag.load());
}

TEST_F(ConnectionFlowTest, HandleCountdownTick)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ServerToClient);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::CountdownTick);
    hdr.payloadSize = 1;

    auto encoded = hdr.encode();
    std::vector<std::uint8_t> pkt(encoded.begin(), encoded.end());
    pkt.push_back(5);

    auto crc = PacketHeader::crc32(pkt.data(), pkt.size());
    pkt.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    pkt.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    pkt.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    pkt.push_back(static_cast<std::uint8_t>(crc & 0xFF));

    rawQueue.push(pkt);
    handler->poll();

    EXPECT_EQ(countdownValue.load(), 5);
}
