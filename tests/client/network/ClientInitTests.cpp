#include "network/ClientInit.hpp"
#include "network/PacketHeader.hpp"

#include <gtest/gtest.h>

TEST(ClientInitTest, BuildClientHello)
{
    std::uint16_t sequence = 42;
    auto pkt               = buildClientHello(sequence);

    EXPECT_EQ(pkt.size(), 21);

    auto hdrOpt = PacketHeader::decode(pkt.data(), pkt.size());
    ASSERT_TRUE(hdrOpt.has_value());
    auto& hdr = *hdrOpt;

    EXPECT_EQ(hdr.packetType, static_cast<std::uint8_t>(PacketType::ClientToServer));
    EXPECT_EQ(hdr.messageType, static_cast<std::uint8_t>(MessageType::ClientHello));
    EXPECT_EQ(hdr.sequenceId, sequence);
    EXPECT_EQ(hdr.payloadSize, 0);
}
