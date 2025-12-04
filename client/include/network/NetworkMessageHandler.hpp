#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/LevelInitData.hpp"
#include "network/PacketHeader.hpp"
#include "network/SnapshotParser.hpp"

#include <cstdint>
#include <optional>
#include <vector>

class NetworkMessageHandler
{
  public:
    NetworkMessageHandler(ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue,
                          ThreadSafeQueue<SnapshotParseResult>& snapshotQueue,
                          ThreadSafeQueue<LevelInitData>& levelInitQueue);

    void poll();

  private:
    std::optional<PacketHeader> decodeHeader(const std::vector<std::uint8_t>& data) const;
    void handleSnapshot(const std::vector<std::uint8_t>& data);
    void handleLevelInit(const std::vector<std::uint8_t>& data);
    void dispatch(const std::vector<std::uint8_t>& data);

    ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue_;
    ThreadSafeQueue<SnapshotParseResult>& snapshotQueue_;
    ThreadSafeQueue<LevelInitData>& levelInitQueue_;
};
