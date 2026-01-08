#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/Window.hpp"
#include "systems/ISystem.hpp"
#include "ui/NotificationData.hpp"

#include <deque>

class NotificationSystem : public ISystem
{
  public:
    NotificationSystem(Window& window, FontManager& fontManager, ThreadSafeQueue<NotificationData>& broadcastQueue);
    ~NotificationSystem() override;

    void update(Registry& registry, float dt) override;

  private:
    void render(const NotificationData& notif, float alpha, float scale, float yOffset);

    Window& window_;
    FontManager& fontManager_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;
    std::deque<NotificationData> activeNotifications_;
};
