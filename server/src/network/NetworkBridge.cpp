#include "network/NetworkBridge.hpp"

#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"

NetworkBridge::NetworkBridge(SendThread& sendThread) : sendThread_(sendThread) {}

void NetworkBridge::processEvents(const std::vector<GameEvent>& events)
{
    for (const auto& event : events) {
        std::visit(
            [this](auto&& evt) {
                using T = std::decay_t<decltype(evt)>;
                if constexpr (std::is_same_v<T, EntitySpawnedEvent>) {
                    EntitySpawnPacket pkt{};
                    pkt.entityId   = evt.entityId;
                    pkt.entityType = evt.entityType;
                    pkt.posX       = evt.posX;
                    pkt.posY       = evt.posY;
                    sendThread_.broadcast(pkt);
                    knownEntities_.insert(evt.entityId);
                } else if constexpr (std::is_same_v<T, EntityDestroyedEvent>) {
                    EntityDestroyedPacket pkt{};
                    pkt.entityId = evt.entityId;
                    sendThread_.broadcast(pkt);
                    knownEntities_.erase(evt.entityId);
                } else if constexpr (std::is_same_v<T, CollisionEvent>) {
                    // Could be used for sound/FX in the future
                }
            },
            event);
    }
}

void NetworkBridge::clear()
{
    knownEntities_.clear();
}
