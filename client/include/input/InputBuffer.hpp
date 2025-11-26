#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>

struct InputCommand
{
    std::uint16_t flags      = 0;
    std::uint32_t sequenceId = 0;
    float posX               = 0.0F;
    float posY               = 0.0F;
    float angle              = 0.0F;
};

class InputBuffer
{
  public:
    void push(const InputCommand& cmd);
    bool tryPop(InputCommand& out);
    std::optional<InputCommand> pop();

  private:
    std::queue<InputCommand> queue_;
    std::mutex mutex_;
};
