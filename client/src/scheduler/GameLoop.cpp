#include "scheduler/GameLoop.hpp"

#include "Logger.hpp"
#include "network/PacketHeader.hpp"

#include <SFML/System/Clock.hpp>
#include <iostream>

int GameLoop::run(Window& window, Registry& registry, UdpSocket* networkSocket, const IpEndpoint* serverEndpoint,
                  std::atomic<bool>& runningFlag)
{
    sf::Clock clock;

    while (window.isOpen() && runningFlag) {
        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                runningFlag = false;
                window.close();
            }
        });

        const float deltaTime = std::min(clock.restart().asSeconds(), 0.1F);

        window.clear();
        ClientScheduler::update(registry, deltaTime);
        window.display();
    }

    if (networkSocket != nullptr && serverEndpoint != nullptr) {
        PacketHeader header{};
        header.packetType  = static_cast<std::uint8_t>(PacketType::ClientToServer);
        header.messageType = static_cast<std::uint8_t>(MessageType::ClientDisconnect);
        header.sequenceId  = 0;
        header.payloadSize = 0;

        auto packet = header.encode();
        networkSocket->sendTo(packet.data(), packet.size(), *serverEndpoint);
        Logger::instance().info("Sent CLIENT_DISCONNECT to server");
    }

    ClientScheduler::stop();
    return 0;
}
