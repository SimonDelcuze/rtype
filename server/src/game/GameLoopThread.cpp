#include "game/GameLoopThread.hpp"

#include <chrono>
#include <thread>

GameLoopThread::GameLoopThread(ThreadSafeQueue<ReceivedInput>& inputs, TickCallback tick, double tickRateHz)
    : inputs_(inputs), tick_(std::move(tick)), period_(1.0 / tickRateHz)
{}

GameLoopThread::~GameLoopThread()
{
    stop();
}

bool GameLoopThread::start()
{
    if (running_)
        return false;
    running_ = true;
    worker_  = std::thread([this] { run(); });
    return true;
}

void GameLoopThread::stop()
{
    if (!running_)
        return;
    running_ = false;
    if (worker_.joinable())
        worker_.join();
}

bool GameLoopThread::isRunning() const
{
    return running_;
}

void GameLoopThread::run()
{
    auto nextTick = std::chrono::steady_clock::now() + period_;
    while (running_) {
        TickInputs batch;
        ReceivedInput item{};
        while (inputs_.tryPop(item)) {
            batch.push_back(std::move(item));
        }
        tick_(batch);
        auto now = std::chrono::steady_clock::now();
        if (now < nextTick) {
            std::this_thread::sleep_until(nextTick);
            nextTick += period_;
        } else {
            nextTick = now + period_;
        }
    }
}
