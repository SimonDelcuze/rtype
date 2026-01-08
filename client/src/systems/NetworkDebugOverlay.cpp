#include "systems/NetworkDebugOverlay.hpp"

#include "ClientRuntime.hpp"
#include "components/NetworkStatsComponent.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

#include <iomanip>
#include <sstream>

NetworkDebugOverlay::NetworkDebugOverlay(Window& window, FontManager& fonts) : window_(window), fonts_(fonts) {}

void NetworkDebugOverlay::initialize() {}

void NetworkDebugOverlay::update(Registry& registry, float)
{
    if (!g_networkDebugEnabled) {
        return;
    }

    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    auto font = fonts_.get("ui");
    if (font == nullptr) {
        return;
    }

    NetworkStatsComponent* stats = nullptr;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (registry.isAlive(id) && registry.has<NetworkStatsComponent>(id)) {
            stats = &registry.get<NetworkStatsComponent>(id);
            break;
        }
    }

    if (stats == nullptr) {
        return;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "=== NETWORK DEBUG ===\n";
    oss << "Ping:      " << stats->currentPing << " ms\n";
    oss << "Avg Ping:  " << stats->averagePing << " ms\n";
    oss << "Min/Max:   " << stats->minPing << " / " << stats->maxPing << " ms\n";
    oss << "Jitter:    " << stats->jitter << " ms\n";
    oss << "\n";
    oss << "Packets RX:  " << stats->packetsReceived << "\n";
    oss << "Packets TX:  " << stats->packetsSent << "\n";
    oss << "Packet Loss: " << stats->packetLossRate << " %\n";
    oss << "\n";
    oss << "BW In:     " << stats->bandwidthIn << " KB/s\n";
    oss << "BW Out:    " << stats->bandwidthOut << " KB/s\n";

    std::string statsText = oss.str();

    window_.drawRectangle({300.0F, 250.0F}, {10.0F, 10.0F}, 0.0F, {1.0F, 1.0F}, Color{0, 0, 0, 200},
                          Color{100, 100, 100, 255}, 2.0F);

    GraphicsFactory factory;
    float yPos = 20.0F;
    std::istringstream iss(statsText);
    std::string line;

    while (std::getline(iss, line)) {
        auto text = factory.createText();
        text->setFont(*font);
        text->setString(line);
        text->setCharacterSize(14);
        text->setPosition(Vector2f{20.0F, yPos});

        if (line.find("Ping:") != std::string::npos || line.find("Avg Ping:") != std::string::npos) {
            if (stats->currentPing < 50.0F) {
                text->setFillColor(Color::Green);
            } else if (stats->currentPing < 100.0F) {
                text->setFillColor(Color{255, 255, 0});
            } else {
                text->setFillColor(Color::Red);
            }
        } else if (line.find("Packet Loss:") != std::string::npos) {
            if (stats->packetLossRate < 1.0F) {
                text->setFillColor(Color::Green);
            } else if (stats->packetLossRate < 5.0F) {
                text->setFillColor(Color{255, 255, 0});
            } else {
                text->setFillColor(Color::Red);
            }
        } else {
            text->setFillColor(Color::White);
        }

        window_.draw(*text);
        yPos += 18.0F;
    }
}

void NetworkDebugOverlay::cleanup() {}
