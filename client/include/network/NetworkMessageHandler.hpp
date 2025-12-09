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
                          ThreadSafeQueue<LevelInitData>& levelInitQueue, std::atomic<bool>* handshakeFlag = nullptr,
                          std::atomic<bool>* allReadyFlag = nullptr, std::atomic<int>* countdownValueFlag = nullptr,
                          std::atomic<bool>* gameStartFlag = nullptr, std::atomic<bool>* joinDeniedFlag = nullptr,
                          std::atomic<bool>* joinAcceptedFlag = nullptr);

    void poll();

  private:
    std::optional<PacketHeader> decodeHeader(const std::vector<std::uint8_t>& data) const;
    void handleSnapshot(const std::vector<std::uint8_t>& data);
    void handleLevelInit(const std::vector<std::uint8_t>& data);
    void handleCountdownTick(const std::vector<std::uint8_t>& data);
    void dispatch(const std::vector<std::uint8_t>& data);

    ThreadSafeQueue<std::vector<std::uint8_t>>& rawQueue_;
    ThreadSafeQueue<SnapshotParseResult>& snapshotQueue_;
    ThreadSafeQueue<LevelInitData>& levelInitQueue_;
    std::atomic<bool>* handshakeFlag_;
    std::atomic<bool>* allReadyFlag_;
    std::atomic<int>* countdownValueFlag_;
    std::atomic<bool>* gameStartFlag_;
    std::atomic<bool>* joinDeniedFlag_;
    std::atomic<bool>* joinAcceptedFlag_;
};
