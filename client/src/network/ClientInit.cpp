#include "network/ClientInit.hpp"

#include "Logger.hpp"

#include <iostream>

std::vector<std::uint8_t> buildClientHello(std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = static_cast<std::uint8_t>(MessageType::ClientHello);
    hdr.sequenceId  = sequence;
    hdr.tickId      = 0;
    hdr.payloadSize = 0;
    auto encoded    = hdr.encode();
    std::vector<std::uint8_t> out(encoded.begin(), encoded.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    out.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(crc & 0xFF));
    return out;
}

void sendClientHelloOnce(const IpEndpoint& server, UdpSocket& socket)
{
    auto pkt = buildClientHello(0);
    socket.sendTo(pkt.data(), pkt.size(), server);
}

std::vector<std::uint8_t> buildSimplePacket(std::uint8_t messageType, std::uint16_t sequence)
{
    PacketHeader hdr{};
    hdr.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
    hdr.messageType = messageType;
    hdr.sequenceId  = sequence;
    hdr.tickId      = 0;
    hdr.payloadSize = 0;
    auto encoded    = hdr.encode();
    std::vector<std::uint8_t> out(encoded.begin(), encoded.end());
    auto crc = PacketHeader::crc32(out.data(), out.size());
    out.push_back(static_cast<std::uint8_t>((crc >> 24) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((crc >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((crc >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(crc & 0xFF));
    return out;
}

void sendJoinRequestOnce(const IpEndpoint& server, std::uint16_t sequence, UdpSocket& socket)
{
    auto pkt = buildSimplePacket(static_cast<std::uint8_t>(MessageType::ClientJoinRequest), sequence);
    socket.sendTo(pkt.data(), pkt.size(), server);
}

void sendClientReadyOnce(const IpEndpoint& server, std::uint16_t sequence, UdpSocket& socket)
{
    auto pkt = buildSimplePacket(static_cast<std::uint8_t>(MessageType::ClientReady), sequence);
    socket.sendTo(pkt.data(), pkt.size(), server);
}

void sendPingOnce(const IpEndpoint& server, std::uint16_t sequence, UdpSocket& socket)
{
    auto pkt = buildSimplePacket(static_cast<std::uint8_t>(MessageType::ClientPing), sequence);
    socket.sendTo(pkt.data(), pkt.size(), server);
}

void sendWelcomeLoop(const IpEndpoint& server, std::atomic<bool>& stopFlag, UdpSocket& socket)
{
    std::uint16_t seq = 0;
    while (!stopFlag.load()) {
        sendClientHelloOnce(server, socket);
        sendJoinRequestOnce(server, ++seq, socket);
        sendPingOnce(server, ++seq, socket);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void sendClientReady(const IpEndpoint& server, UdpSocket& socket)
{
    static std::atomic<std::uint16_t> readySeq{1000};
    sendClientReadyOnce(server, readySeq++, socket);
    Logger::instance().info("CLIENT_READY sent to server");
}

bool startReceiver(NetPipelines& net, std::uint16_t port, std::atomic<bool>& handshakeFlag)
{
    if (net.socket == nullptr) {
        net.socket = std::make_shared<UdpSocket>();
    }
    if (!net.socket->isOpen()) {
        if (!net.socket->open(IpEndpoint::v4(0, 0, 0, 0, port))) {
            std::cerr << "Failed to open shared socket on port " << port << '\n';
            return false;
        }
    }
    net.receiver = std::make_unique<NetworkReceiver>(
        IpEndpoint::v4(0, 0, 0, 0, port), [&](std::vector<std::uint8_t>&& packet) { net.raw.push(std::move(packet)); },
        net.socket);
    if (!net.receiver->start()) {
        std::cerr << "Failed to start NetworkReceiver on port " << port << '\n';
        return false;
    }
    net.handler = std::make_unique<NetworkMessageHandler>(
        net.raw, net.parsed, net.levelInit, net.levelEvents, net.spawns, net.destroys, &handshakeFlag, &net.allReady,
        &net.countdownValue, &net.gameStartReceived, &net.joinDenied, &net.joinAccepted);
    Logger::instance().info("Receiver started on port " + std::to_string(port));
    return true;
}

bool startSender(NetPipelines& net, InputBuffer& inputBuffer, std::uint16_t clientId, const IpEndpoint& server)
{
    net.sender = std::make_unique<NetworkSender>(
        inputBuffer, server, clientId, std::chrono::milliseconds(16), IpEndpoint::v4(0, 0, 0, 0, 0),
        [](const IError& err) { std::cerr << "NetworkSender error: " << err.what() << '\n'; }, net.socket);
    if (!net.sender->start()) {
        std::cerr << "Failed to start NetworkSender\n";
        return false;
    }
    Logger::instance().info("Sender started to " + std::to_string(static_cast<int>(server.addr[0])) + "." +
                            std::to_string(static_cast<int>(server.addr[1])) + "." +
                            std::to_string(static_cast<int>(server.addr[2])) + "." +
                            std::to_string(static_cast<int>(server.addr[3])) + ":" + std::to_string(server.port) +
                            " as clientId=" + std::to_string(clientId));
    return true;
}
