#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "input/InputBuffer.hpp"
#include "network/LevelInitData.hpp"
#include "network/NetworkMessageHandler.hpp"
#include "network/NetworkReceiver.hpp"
#include "network/NetworkSender.hpp"
#include "network/PacketHeader.hpp"

#include <cstdint>
#include <memory>
#include <vector>

struct NetPipelines
{
    ThreadSafeQueue<std::vector<std::uint8_t>> raw;
    ThreadSafeQueue<SnapshotParseResult> parsed;
    ThreadSafeQueue<LevelInitData> levelInit;
    std::shared_ptr<UdpSocket> socket;
    std::unique_ptr<NetworkReceiver> receiver;
    std::unique_ptr<NetworkMessageHandler> handler;
    std::unique_ptr<NetworkSender> sender;
};

#include <atomic>

std::vector<std::uint8_t> buildClientHello(std::uint16_t sequence);
void sendClientHelloOnce(const IpEndpoint& server, UdpSocket& socket);
void sendJoinRequestOnce(const IpEndpoint& server, std::uint16_t sequence, UdpSocket& socket);
void sendClientReadyOnce(const IpEndpoint& server, std::uint16_t sequence, UdpSocket& socket);
void sendPingOnce(const IpEndpoint& server, std::uint16_t sequence, UdpSocket& socket);
void sendWelcomeLoop(const IpEndpoint& server, std::atomic<bool>& stopFlag, UdpSocket& socket);
bool startReceiver(NetPipelines& net, std::uint16_t port, std::atomic<bool>& handshakeFlag);
bool startSender(NetPipelines& net, InputBuffer& inputBuffer, std::uint16_t clientId, const IpEndpoint& server);
