#include "network/UdpSocket.hpp"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

static IpEndpoint loopback(std::uint16_t port)
{
    return IpEndpoint::v4(127, 0, 0, 1, port);
}

TEST(UdpSocket, NonBlockingReceiveWouldBlock)
{
    UdpSocket s;
    ASSERT_TRUE(s.open(loopback(0)));
    std::array<std::uint8_t, 1024> buf{};
    IpEndpoint src{};
    auto r = s.recvFrom(buf.data(), buf.size(), src);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.error, UdpError::WouldBlock);
}

TEST(UdpSocket, SendReceiveLoopback)
{
    UdpSocket rx;
    ASSERT_TRUE(rx.open(loopback(0)));
    auto rxEp = rx.localEndpoint();
    ASSERT_NE(rxEp.port, 0);

    UdpSocket tx;
    ASSERT_TRUE(tx.open(loopback(0)));

    std::vector<std::uint8_t> payload{1, 2, 3, 4, 5, 6, 7};
    auto sr = tx.sendTo(payload.data(), payload.size(), loopback(rxEp.port));
    ASSERT_TRUE(sr.ok());
    ASSERT_EQ(sr.size, payload.size());

    std::array<std::uint8_t, 64> buf{};
    IpEndpoint src{};
    UdpResult rr{0, UdpError::WouldBlock};
    for (int i = 0; i < 1000; ++i) {
        rr = rx.recvFrom(buf.data(), buf.size(), src);
        if (rr.ok())
            break;
        if (rr.error != UdpError::WouldBlock)
            break;
    }
    ASSERT_TRUE(rr.ok());
    ASSERT_EQ(rr.size, payload.size());
    for (std::size_t i = 0; i < payload.size(); ++i) {
        EXPECT_EQ(buf[i], payload[i]);
    }
    EXPECT_EQ(src.addr[0], 127);
}
