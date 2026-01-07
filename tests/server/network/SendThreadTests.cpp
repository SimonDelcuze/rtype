#include "network/SendThread.hpp"

#include <array>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace std::chrono_literals;

static bool pollRecv(UdpSocket& sock, std::array<std::uint8_t, 512>& buf, std::size_t& outSize, int attempts = 200)
{
    IpEndpoint src{};
    for (int i = 0; i < attempts; ++i) {
        auto r = sock.recvFrom(buf.data(), buf.size(), src);
        if (r.ok()) {
            outSize = r.size;
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

TEST(SendThread, BroadcastsToAllClients)
{
    UdpSocket c1;
    UdpSocket c2;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c2.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    auto ep1 = c1.localEndpoint();
    auto ep2 = c2.localEndpoint();

    SendThread st(IpEndpoint::v4(127, 0, 0, 1, 0), {ep1, ep2}, 60.0);
    ASSERT_TRUE(st.start());

    DeltaStatePacket pkt{};
    pkt.header.sequenceId = 1;
    pkt.header.tickId     = 10;
    pkt.entries.push_back(DeltaEntry{1, 1.0F, 2.0F, 3.0F, 4.0F, 5});
    st.publish(pkt);

    std::array<std::uint8_t, 512> buf{};
    std::size_t size = 0;
    ASSERT_TRUE(pollRecv(c1, buf, size));
    ASSERT_TRUE(pollRecv(c2, buf, size));

    st.stop();
}

TEST(SendThread, SendsLatestPayload)
{
    UdpSocket c;
    ASSERT_TRUE(c.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    auto ep = c.localEndpoint();

    SendThread st(IpEndpoint::v4(127, 0, 0, 1, 0), {ep}, 120.0);
    ASSERT_TRUE(st.start());

    DeltaStatePacket pkt1{};
    pkt1.header.sequenceId = 1;
    pkt1.header.tickId     = 1;
    pkt1.entries.push_back(DeltaEntry{1, 0.0F, 0.0F, 0.0F, 0.0F, 0});
    st.publish(pkt1);

    DeltaStatePacket pkt2{};
    pkt2.header.sequenceId = 2;
    pkt2.header.tickId     = 2;
    pkt2.entries.push_back(DeltaEntry{2, 1.0F, 1.0F, 1.0F, 1.0F, 1});
    st.publish(pkt2);

    std::array<std::uint8_t, 512> buf{};
    std::size_t size = 0;
    bool gotLatest   = false;
    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(pollRecv(c, buf, size));
        auto decoded = DeltaStatePacket::decode(buf.data(), size);
        ASSERT_TRUE(decoded.has_value());
        if (decoded->header.sequenceId == pkt2.header.sequenceId) {
            gotLatest = true;
            break;
        }
    }
    EXPECT_TRUE(gotLatest);

    st.stop();
}

TEST(SendThread, NoPayloadNoSend)
{
    UdpSocket c;
    ASSERT_TRUE(c.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    auto ep = c.localEndpoint();

    SendThread st(IpEndpoint::v4(127, 0, 0, 1, 0), {ep}, 60.0);
    ASSERT_TRUE(st.start());

    std::array<std::uint8_t, 512> buf{};
    std::size_t size = 0;
    EXPECT_FALSE(pollRecv(c, buf, size, 50));

    st.stop();
}

TEST(SendThread, HandlesClientListUpdates)
{
    UdpSocket c1;
    UdpSocket c2;
    ASSERT_TRUE(c1.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    ASSERT_TRUE(c2.open(IpEndpoint::v4(127, 0, 0, 1, 0)));
    auto ep1 = c1.localEndpoint();
    auto ep2 = c2.localEndpoint();

    SendThread st(IpEndpoint::v4(127, 0, 0, 1, 0), {ep1}, 120.0);
    ASSERT_TRUE(st.start());

    DeltaStatePacket pkt{};
    pkt.header.sequenceId = 3;
    pkt.header.tickId     = 3;
    pkt.entries.push_back(DeltaEntry{3, 3.0F, 3.0F, 3.0F, 3.0F, 3});
    st.publish(pkt);

    std::array<std::uint8_t, 512> buf{};
    std::size_t size = 0;
    ASSERT_TRUE(pollRecv(c1, buf, size));

    st.setClients({ep2});
    buf.fill(0);
    size = 0;
    ASSERT_TRUE(pollRecv(c2, buf, size));

    st.stop();
}
