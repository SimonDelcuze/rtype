#include "scheduler/GameLoop.hpp"

#include "Logger.hpp"
#include "network/PacketHeader.hpp"

#include <SFML/System/Clock.hpp>
#include <chrono>
#include <iostream>

int GameLoop::run(Window& window, Registry& registry, UdpSocket* networkSocket, const IpEndpoint* serverEndpoint,
                  std::atomic<bool>& runningFlag, const std::function<void(const Event&)>& onEvent)
{
    auto lastCheck = std::chrono::steady_clock::now();

    while (window.isOpen() && runningFlag) {
        window.pollEvents([&](const Event& event) {
            if (onEvent) {
                onEvent(event);
            }
            if (event.type == EventType::Closed) {
                runningFlag = false;
                window.close();
            }
        });

        auto now       = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastCheck).count();
        lastCheck      = now;
        deltaTime      = std::min(deltaTime, 0.1F);

        window.clear();
        auto updateStart = std::chrono::steady_clock::now();
        ClientScheduler::update(registry, deltaTime);
        auto updateEnd = std::chrono::steady_clock::now();
        auto updateMs  = std::chrono::duration_cast<std::chrono::milliseconds>(updateEnd - updateStart).count();
        if (updateMs > 30) {
            Logger::instance().warn("[Perf] ClientScheduler update took " + std::to_string(updateMs) +
                                    "ms entities=" + std::to_string(registry.entityCount()));
        }
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
