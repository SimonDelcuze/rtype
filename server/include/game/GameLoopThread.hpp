#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "network/InputReceiveThread.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

class GameLoopThread
{
  public:
    using TickInputs   = std::vector<ReceivedInput>;
    using TickCallback = std::function<void(const TickInputs&)>;

    GameLoopThread(ThreadSafeQueue<ReceivedInput>& inputs, TickCallback tick, double tickRateHz = 60.0);
    ~GameLoopThread();

    bool start();
    void stop();
    bool isRunning() const;

  private:
    void run();

    ThreadSafeQueue<ReceivedInput>& inputs_;
    TickCallback tick_;
    std::chrono::duration<double> period_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};
