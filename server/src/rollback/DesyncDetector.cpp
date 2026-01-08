#include "rollback/DesyncDetector.hpp"

DesyncDetector::DesyncDetector(std::uint32_t checksumInterval, std::uint32_t timeoutThreshold)
    : checksumInterval_(checksumInterval), timeoutThreshold_(timeoutThreshold)
{}

void DesyncDetector::setDesyncCallback(DesyncCallback callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    desyncCallback_ = std::move(callback);
}

void DesyncDetector::reportClientChecksum(EntityId playerId, std::uint64_t tick, std::uint32_t clientChecksum,
                                          std::uint64_t currentTick)
{
    std::lock_guard<std::mutex> lock(clientInfoMutex_);

    auto& info          = clientInfo_[playerId];
    info.lastTick       = tick;
    info.lastChecksum   = clientChecksum;
    info.lastUpdateTime = currentTick;
}

bool DesyncDetector::verifyChecksum(EntityId playerId, std::uint64_t tick, std::uint32_t clientChecksum,
                                    std::uint32_t serverChecksum)
{
    if (clientChecksum == serverChecksum) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(clientInfoMutex_);
        auto it = clientInfo_.find(playerId);
        if (it != clientInfo_.end()) {
            it->second.desyncCount++;
        }
    }

    DesyncInfo info;
    info.playerId       = playerId;
    info.tick           = tick;
    info.type           = DesyncType::CHECKSUM_MISMATCH;
    info.serverChecksum = serverChecksum;
    info.clientChecksum = clientChecksum;
    info.description    = "Client checksum mismatch at tick " + std::to_string(tick);

    triggerDesync(info);
    return false;
}

void DesyncDetector::checkTimeouts(std::uint64_t currentTick)
{
    std::lock_guard<std::mutex> lock(clientInfoMutex_);

    for (const auto& [playerId, info] : clientInfo_) {
        if (currentTick - info.lastUpdateTime > timeoutThreshold_) {
            DesyncInfo desyncInfo;
            desyncInfo.playerId       = playerId;
            desyncInfo.tick           = currentTick;
            desyncInfo.type           = DesyncType::TIMEOUT;
            desyncInfo.serverChecksum = 0;
            desyncInfo.clientChecksum = info.lastChecksum;
            desyncInfo.description =
                "Client checksum timeout (last update: tick " + std::to_string(info.lastUpdateTime) + ")";

            triggerDesync(desyncInfo);
        }
    }
}

void DesyncDetector::removeClient(EntityId playerId)
{
    std::lock_guard<std::mutex> lock(clientInfoMutex_);
    clientInfo_.erase(playerId);
}

std::uint32_t DesyncDetector::getDesyncCount(EntityId playerId) const
{
    std::lock_guard<std::mutex> lock(clientInfoMutex_);
    auto it = clientInfo_.find(playerId);
    if (it != clientInfo_.end()) {
        return it->second.desyncCount;
    }
    return 0;
}

void DesyncDetector::resetDesyncCount(EntityId playerId)
{
    std::lock_guard<std::mutex> lock(clientInfoMutex_);
    auto it = clientInfo_.find(playerId);
    if (it != clientInfo_.end()) {
        it->second.desyncCount = 0;
    }
}

void DesyncDetector::clear()
{
    std::lock_guard<std::mutex> lock(clientInfoMutex_);
    clientInfo_.clear();
}

void DesyncDetector::triggerDesync(const DesyncInfo& info)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (desyncCallback_) {
        desyncCallback_(info);
    }
}
