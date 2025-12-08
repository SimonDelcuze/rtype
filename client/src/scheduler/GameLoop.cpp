#include "scheduler/GameLoop.hpp"

#include "network/PacketHeader.hpp"

#include <SFML/System/Clock.hpp>
#include <iostream>

int GameLoop::run(Window& window, Registry& registry, UdpSocket* networkSocket, const IpEndpoint* serverEndpoint)
{
    sf::Clock clock;

    while (window.isOpen()) {
        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                if (networkSocket != nullptr && serverEndpoint != nullptr) {
                    PacketHeader header{};
                    header.packetType = static_cast<std::uint8_t>(PacketType::ClientToServer);
                    header.messageType = static_cast<std::uint8_t>(MessageType::ClientDisconnect);
                    header.sequenceId = 0;
                    header.payloadSize = 0;

                    auto packet = header.encode();
                    networkSocket->sendTo(packet.data(), packet.size(), *serverEndpoint);
                }
                window.close();
            }
        });

        const float deltaTime = clock.restart().asSeconds();

        window.clear();
        ClientScheduler::update(registry, deltaTime);
        window.display();
    }

    ClientScheduler::stop();
    return 0;
}
