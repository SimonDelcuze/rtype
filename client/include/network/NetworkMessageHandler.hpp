#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/LevelInitData.hpp"
#include "network/PacketHeader.hpp"
#include "network/SnapshotParser.hpp"

#include <atomic>
#include <cstdint>
#include <optional>
#include <vector>

class NetworkMessageHandler
{
  public:
    NetworkMessageHandler(ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue,
                          ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
                          ThreadSafeQueue<LevelInitData>& levelInitQueue, std::atomic<bool>* handshakeFlag = nullptr);

    void poll();

  private:
    std::optional<PacketHeader> decodeHeader(const std::vector<std::uint8_t>& data) const;
    void handleSnapshot(const std::vector<std::uint8_t>& data);
    void handleLevelInit(const std::vector<std::uint8_t>& data);
    void dispatch(const std::vector<std::uint8_t>& data);

    ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue_;
    ThreadSafeQueue<SnapshotParseResult>& snapshotQueue_;
    ThreadSafeQueue<LevelInitData>& levelInitQueue_;
    std::atomic<bool>* handshakeFlag_;
};
