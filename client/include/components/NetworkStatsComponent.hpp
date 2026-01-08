#pragma once

#include <cmath>
#include <cstdint>
#include <deque>

struct NetworkStatsComponent
{
    float currentPing             = 0.0F;
    float averagePing             = 0.0F;
    float minPing                 = 999.0F;
    float maxPing                 = 0.0F;
    float jitter                  = 0.0F;
    std::uint32_t packetsSent     = 0;
    std::uint32_t packetsReceived = 0;
    std::uint32_t packetsLost     = 0;
    float packetLossRate          = 0.0F;
    float bandwidthIn             = 0.0F;
    float bandwidthOut            = 0.0F;
    float lastUpdateTime          = 0.0F;
    float timeSinceUpdate         = 0.0F;
    std::deque<float> pingHistory;
    std::deque<std::uint32_t> bytesInHistory;
    std::deque<std::uint32_t> bytesOutHistory;
    static constexpr std::size_t kMaxHistorySize = 60;

    static NetworkStatsComponent create()
    {
        return NetworkStatsComponent{};
    }

    void addPingSample(float ping)
    {
        currentPing = ping;
        pingHistory.push_back(ping);
        if (pingHistory.size() > kMaxHistorySize) {
            pingHistory.pop_front();
        }

        if (ping < minPing)
            minPing = ping;
        if (ping > maxPing)
            maxPing = ping;

        float sum = 0.0F;
        for (float p : pingHistory) {
            sum += p;
        }
        averagePing = sum / static_cast<float>(pingHistory.size());

        if (pingHistory.size() > 1) {
            float variance = 0.0F;
            for (float p : pingHistory) {
                float diff = p - averagePing;
                variance += diff * diff;
            }
            jitter = std::sqrt(variance / static_cast<float>(pingHistory.size()));
        }
    }

    void addBandwidthSample(std::uint32_t bytesIn, std::uint32_t bytesOut)
    {
        bytesInHistory.push_back(bytesIn);
        bytesOutHistory.push_back(bytesOut);

        if (bytesInHistory.size() > kMaxHistorySize) {
            bytesInHistory.pop_front();
        }
        if (bytesOutHistory.size() > kMaxHistorySize) {
            bytesOutHistory.pop_front();
        }

        std::uint32_t totalIn  = 0;
        std::uint32_t totalOut = 0;

        for (std::uint32_t b : bytesInHistory) {
            totalIn += b;
        }
        for (std::uint32_t b : bytesOutHistory) {
            totalOut += b;
        }

        bandwidthIn  = static_cast<float>(totalIn) / 1024.0F;
        bandwidthOut = static_cast<float>(totalOut) / 1024.0F;
    }

    void updatePacketLoss()
    {
        if (packetsSent > 0) {
            packetsLost    = packetsSent - packetsReceived;
            packetLossRate = (static_cast<float>(packetsLost) / static_cast<float>(packetsSent)) * 100.0F;
        }
    }
};
