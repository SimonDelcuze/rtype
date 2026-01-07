#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

using EntityId = std::uint32_t;

enum class DesyncType
{
    CHECKSUM_MISMATCH,   
    ENTITY_COUNT_MISMATCH,
    CRITICAL_FIELD_MISMATCH,
    TIMEOUT
};

struct DesyncInfo
{
    EntityId playerId;
    std::uint64_t tick;
    DesyncType type;
    std::uint32_t serverChecksum;
    std::uint32_t clientChecksum;
    std::string description;
};

using DesyncCallback = std::function<void(const DesyncInfo&)>;

struct ClientChecksumInfo
{
    std::uint64_t lastTick = 0;
    std::uint32_t lastChecksum = 0;
    std::uint64_t lastUpdateTime = 0;
    std::uint32_t desyncCount = 0;
};

class DesyncDetector
{
  public:
    DesyncDetector(std::uint32_t checksumInterval = 60, std::uint32_t timeoutThreshold = 180);

    void setDesyncCallback(DesyncCallback callback);

    void reportClientChecksum(EntityId playerId, std::uint64_t tick,
                              std::uint32_t clientChecksum, std::uint64_t currentTick);

    bool verifyChecksum(EntityId playerId, std::uint64_t tick, std::uint32_t clientChecksum,
                        std::uint32_t serverChecksum);

    void checkTimeouts(std::uint64_t currentTick);

    void removeClient(EntityId playerId);

    std::uint32_t getDesyncCount(EntityId playerId) const;

    void resetDesyncCount(EntityId playerId);

    std::uint32_t getChecksumInterval() const
    {
        return checksumInterval_;
    }

    bool shouldVerifyChecksum(std::uint64_t tick) const
    {
        return (tick % checksumInterval_) == 0;
    }

    void clear();

  private:
    void triggerDesync(const DesyncInfo& info);

    std::uint32_t checksumInterval_;
    std::uint32_t timeoutThreshold_;

    std::unordered_map<EntityId, ClientChecksumInfo> clientInfo_;
    mutable std::mutex clientInfoMutex_;

    DesyncCallback desyncCallback_;
    std::mutex callbackMutex_;
};
