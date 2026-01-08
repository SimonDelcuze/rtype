#include "Logger.hpp"
#include "game/GameInstance.hpp"
#include "network/DesyncDetectedPacket.hpp"
#include "network/RollbackRequestPacket.hpp"

void GameInstance::captureStateSnapshot()
{
    std::uint32_t checksum = rollbackManager_.captureState(currentTick_, registry_);

    if (currentTick_ % 60 == 0) {
        Logger::instance().info("[Rollback] Captured state snapshot at tick " + std::to_string(currentTick_) +
                                " checksum=0x" + std::to_string(checksum));
    }
}

void GameInstance::handleDesync(const DesyncInfo& desyncInfo)
{
    Logger::instance().warn("[Rollback] Desynchronization detected for player " + std::to_string(desyncInfo.playerId) +
                            " at tick " + std::to_string(desyncInfo.tick) + ": " + desyncInfo.description);

    IpEndpoint targetClient;
    bool found = false;
    for (const auto& [key, session] : sessions_) {
        if (session.playerId == desyncInfo.playerId) {
            targetClient = session.endpoint;
            found        = true;
            break;
        }
    }

    if (!found) {
        Logger::instance().warn("[Rollback] Could not find client for player " + std::to_string(desyncInfo.playerId));
        return;
    }

    DesyncDetectedPacket desyncPacket{};
    desyncPacket.header.packetType = static_cast<std::uint8_t>(PacketType::ServerToClient);
    desyncPacket.playerId          = desyncInfo.playerId;
    desyncPacket.tick              = desyncInfo.tick;
    desyncPacket.desyncType        = static_cast<std::uint8_t>(desyncInfo.type);
    desyncPacket.expectedChecksum  = desyncInfo.serverChecksum;
    desyncPacket.actualChecksum    = desyncInfo.clientChecksum;

    auto desyncData = desyncPacket.encode();
    std::vector<std::uint8_t> desyncVec(desyncData.begin(), desyncData.end());
    sendThread_.sendTo(desyncVec, targetClient);

    std::uint64_t rollbackToTick = desyncInfo.tick;
    auto tickRange               = rollbackManager_.getTickRange();

    if (tickRange.has_value()) {
        std::uint64_t safeRollbackTick = (desyncInfo.tick > 10) ? (desyncInfo.tick - 10) : 0;
        std::uint64_t oldestTick       = tickRange->first;

        if (safeRollbackTick < oldestTick) {
            safeRollbackTick = oldestTick;
        }

        if (rollbackManager_.canRollbackTo(safeRollbackTick)) {
            rollbackToTick = safeRollbackTick;
        } else {
            rollbackToTick = oldestTick;
        }
    }

    Logger::instance().info("[Rollback] Requesting client rollback to tick " + std::to_string(rollbackToTick) +
                            " (current: " + std::to_string(currentTick_) + ")");

    RollbackRequestPacket rollbackPacket{};
    rollbackPacket.playerId       = desyncInfo.playerId;
    rollbackPacket.rollbackToTick = rollbackToTick;
    rollbackPacket.currentTick    = currentTick_;
    rollbackPacket.reason         = static_cast<std::uint8_t>(desyncInfo.type);

    auto rollbackData = rollbackPacket.encode();
    std::vector<std::uint8_t> rollbackVec(rollbackData.begin(), rollbackData.end());
    sendThread_.sendTo(rollbackVec, targetClient);

    desyncDetector_.resetDesyncCount(desyncInfo.playerId);
}
