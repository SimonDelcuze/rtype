#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/EntityDestroyedPacket.hpp"
#include "network/EntitySpawnPacket.hpp"
#include "network/LevelEventData.hpp"
#include "network/LevelInitData.hpp"
#include "network/PacketHeader.hpp"
#include "network/SnapshotParser.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

class NetworkMessageHandler
{
  public:
    NetworkMessageHandler(
        ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue, ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
        ThreadSafeQueue<LevelInitData>& levelInitQueue, ThreadSafeQueue<LevelEventData>& levelEventQueue,
        ThreadSafeQueue<EntitySpawnPacket>& spawnQueue, ThreadSafeQueue<EntityDestroyedPacket>& destroyQueue,
        ThreadSafeQueue<std::string>* disconnectQueue = nullptr, ThreadSafeQueue<std::string>* broadcastQueue = nullptr,
        std::atomic<bool>* handshakeFlag = nullptr, std::atomic<bool>* allReadyFlag = nullptr,
        std::atomic<int>* countdownValueFlag = nullptr, std::atomic<bool>* gameStartFlag = nullptr,
        std::atomic<bool>* joinDeniedFlag = nullptr, std::atomic<bool>* joinAcceptedFlag = nullptr);
    NetworkMessageHandler(ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue,
                          ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
                          ThreadSafeQueue<LevelInitData>& levelInitQueue);

    void poll();

  private:
    std::optional<PacketHeader> decodeHeader(const std::vector<std::uint8_t>& data) const;
    void handleSnapshot(const std::vector<std::uint8_t>& data);
    void handleSnapshotChunk(const PacketHeader& hdr, const std::vector<std::uint8_t>& data);
    void handleEntitySpawn(const std::vector<std::uint8_t>& data);
    void handleEntityDestroyed(const std::vector<std::uint8_t>& data);
    void handleLevelInit(const std::vector<std::uint8_t>& data);
    void handleLevelEvent(const std::vector<std::uint8_t>& data);
    void handleCountdownTick(const std::vector<std::uint8_t>& data);
    void handleServerDisconnect(const std::vector<std::uint8_t>& data);
    void handleServerBroadcast(const std::vector<std::uint8_t>& data);
    void dispatch(const std::vector<std::uint8_t>& data);

    struct ChunkAccumulator
    {
        PacketHeader headerTemplate{};
        std::uint16_t totalChunks   = 0;
        std::uint16_t received      = 0;
        std::uint16_t totalEntities = 0;
        std::vector<std::vector<std::uint8_t>> parts;
        std::chrono::steady_clock::time_point lastUpdate{};
    };

    ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue_;
    ThreadSafeQueue<SnapshotParseResult>& snapshotQueue_;
    ThreadSafeQueue<LevelInitData>& levelInitQueue_;
    ThreadSafeQueue<LevelEventData>& levelEventQueue_;
    ThreadSafeQueue<EntitySpawnPacket>& spawnQueue_;
    ThreadSafeQueue<EntityDestroyedPacket>& destroyQueue_;
    std::atomic<bool>* handshakeFlag_;
    std::atomic<bool>* allReadyFlag_;
    std::atomic<int>* countdownValueFlag_;
    std::atomic<bool>* gameStartFlag_;
    std::atomic<bool>* joinDeniedFlag_;
    std::atomic<bool>* joinAcceptedFlag_;
    ThreadSafeQueue<std::string>* disconnectQueue_;
    ThreadSafeQueue<std::string>* broadcastQueue_;
    std::map<std::uint32_t, ChunkAccumulator> chunkAccumulators_;
};
