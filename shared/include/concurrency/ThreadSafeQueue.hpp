#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

template <typename T> class ThreadSafeQueue
{
  public:
    ThreadSafeQueue()                                  = default;
    ThreadSafeQueue(const ThreadSafeQueue&)            = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(const T& value)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(value);
        }
        _cv.notify_one();
    }

    void push(T&& value)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(std::move(value));
        }
        _cv.notify_one();
    }

    bool tryPop(T& out)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return false;
        }
        out = std::move(_queue.front());
        _queue.pop();
        return true;
    }

    T waitPop()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this] { return !_queue.empty(); });
        T value = std::move(_queue.front());
        _queue.pop();
        return value;
    }

  private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
};
