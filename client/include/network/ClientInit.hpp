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
    std::unique_ptr<NetworkReceiver> receiver;
    std::unique_ptr<NetworkMessageHandler> handler;
    std::unique_ptr<NetworkSender> sender;
};

#include <atomic>

std::vector<std::uint8_t> buildClientHello(std::uint16_t sequence);
void sendClientHelloOnce(const IpEndpoint& server);
void sendJoinRequestOnce(const IpEndpoint& server, std::uint16_t sequence);
void sendClientReadyOnce(const IpEndpoint& server, std::uint16_t sequence);
void sendPingOnce(const IpEndpoint& server, std::uint16_t sequence);
void sendWelcomeLoop(const IpEndpoint& server, std::atomic<bool>& stopFlag);
bool startReceiver(NetPipelines& net, std::uint16_t port, std::atomic<bool>& handshakeFlag);
bool startSender(NetPipelines& net, InputBuffer& inputBuffer, std::uint16_t clientId, const IpEndpoint& server);
