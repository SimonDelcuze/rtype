#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class EventBus
{
  public:
    EventBus() = default;

    template <typename T, typename F> void subscribe(F&& f)
    {
        auto& ch = ensureChannel<T>();
        ch.subscribers.emplace_back(std::forward<F>(f));
    }

    template <typename T> void emit(const T& e)
    {
        ensureChannel<T>().push(e);
    }

    template <typename T> void emit(T&& e)
    {
        ensureChannel<T>().push(std::move(e));
    }

    void process()
    {
        for (const auto& ti : order_) {
            auto it = channels_.find(ti);
            if (it != channels_.end()) {
                it->second->process();
            }
        }
    }

    void clear()
    {
        for (auto& kv : channels_) {
            kv.second->clear();
        }
    }

  private:
    struct IChannel
    {
        virtual ~IChannel()    = default;
        virtual void process() = 0;
        virtual void clear()   = 0;
    };

    template <typename T> struct Channel : IChannel
    {
        using Callback = std::function<void(const T&)>;
        std::vector<Callback> subscribers;
        std::vector<T> current;
        std::vector<T> next;

        void push(const T& e)
        {
            next.push_back(e);
        }

        void push(T&& e)
        {
            next.push_back(std::move(e));
        }

        void process() override
        {
            if (next.empty() && current.empty()) {
                return;
            }
            current.swap(next);
            for (const auto& e : current) {
                for (auto& cb : subscribers) {
                    try {
                        cb(e);
                    } catch (...) {
                    }
                }
            }
            current.clear();
        }

        void clear() override
        {
            current.clear();
            next.clear();
        }
    };

    template <typename T> Channel<T>& ensureChannel()
    {
        const std::type_index ti(typeid(T));
        auto it = channels_.find(ti);
        if (it == channels_.end()) {
            auto ptr  = std::make_unique<Channel<T>>();
            auto* raw = ptr.get();
            channels_.emplace(ti, std::move(ptr));
            order_.push_back(ti);
            return *raw;
        }
        return static_cast<Channel<T>&>(*it->second);
    }

    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> channels_;
    std::vector<std::type_index> order_;
};
