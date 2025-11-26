#include "input/InputBuffer.hpp"

void InputBuffer::push(const InputCommand& cmd)
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(cmd);
}

bool InputBuffer::tryPop(InputCommand& out)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    out = queue_.front();
    queue_.pop();
    return true;
}

std::optional<InputCommand> InputBuffer::pop()
{
    InputCommand cmd{};
    if (tryPop(cmd)) {
        return cmd;
    }
    return std::nullopt;
}
