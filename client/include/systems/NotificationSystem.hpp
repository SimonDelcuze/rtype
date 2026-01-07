#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"

#include <deque>
#include <string>

class NotificationSystem : public ISystem
{
  public:
    NotificationSystem(Window& window, FontManager& fonts, ThreadSafeQueue<std::string>& broadcastQueue);
    virtual ~NotificationSystem();

    void update(Registry& registry, float deltaTime) override;

  private:
    struct Notification
    {
        std::string message;
        float timer;
    };

    Window& window_;
    FontManager& fonts_;
    ThreadSafeQueue<std::string>& broadcastQueue_;
    std::deque<Notification> notifications_;
};
