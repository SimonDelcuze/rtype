#pragma once

#include <cstdint>
#include <deque>

struct InputHistoryEntry
{
    std::uint32_t sequenceId = 0;
    std::uint16_t flags      = 0;
    float posX               = 0.0F;
    float posY               = 0.0F;
    float angle              = 0.0F;
    float deltaTime          = 0.016F;
};

struct InputHistoryComponent
{
    std::deque<InputHistoryEntry> history;
    std::uint32_t lastAcknowledgedSequence = 0;
    std::size_t maxHistorySize             = 128;

    void pushInput(std::uint32_t sequenceId, std::uint16_t flags, float posX, float posY, float angle,
                   float deltaTime = 0.016F);

    void acknowledgeUpTo(std::uint32_t sequenceId);

    std::deque<InputHistoryEntry> getInputsAfter(std::uint32_t sequenceId) const;
    void clear();

    std::size_t size() const
    {
        return history.size();
    }
};

inline void InputHistoryComponent::pushInput(std::uint32_t sequenceId, std::uint16_t flags, float posX, float posY,
                                             float angle, float deltaTime)
{
    InputHistoryEntry entry;
    entry.sequenceId = sequenceId;
    entry.flags      = flags;
    entry.posX       = posX;
    entry.posY       = posY;
    entry.angle      = angle;
    entry.deltaTime  = deltaTime;

    history.push_back(entry);

    while (history.size() > maxHistorySize) {
        history.pop_front();
    }
}

inline void InputHistoryComponent::acknowledgeUpTo(std::uint32_t sequenceId)
{
    lastAcknowledgedSequence = sequenceId;

    while (!history.empty() && history.front().sequenceId <= sequenceId) {
        history.pop_front();
    }
}

inline std::deque<InputHistoryEntry> InputHistoryComponent::getInputsAfter(std::uint32_t sequenceId) const
{
    std::deque<InputHistoryEntry> result;

    for (const auto& entry : history) {
        if (entry.sequenceId > sequenceId) {
            result.push_back(entry);
        }
    }

    return result;
}

inline void InputHistoryComponent::clear()
{
    history.clear();
    lastAcknowledgedSequence = 0;
}
