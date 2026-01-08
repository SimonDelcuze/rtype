#include "ui/MenuRunner.hpp"

#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"

MenuRunner::MenuRunner(Window& window, FontManager& fonts, TextureManager& textures, std::atomic<bool>& running,
                       ThreadSafeQueue<NotificationData>& broadcastQueue)
    : window_(window), fonts_(fonts), textures_(textures), runningFlag_(running), broadcastQueue_(broadcastQueue),
      renderSystem_(window), inputFieldSystem_(window, fonts), buttonSystem_(window, fonts),
      hudSystem_(window, fonts, textures), notificationSystem_(window, fonts, broadcastQueue)
{}

Registry& MenuRunner::getRegistry()
{
    return registry_;
}

#include <chrono>

void MenuRunner::runLoop(IMenu& menu)
{
    auto lastTime = std::chrono::steady_clock::now();

    while (window_.isOpen() && !menu.isDone() && runningFlag_) {
        auto currentTime                     = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        lastTime                             = currentTime;
        float dt                             = std::min(elapsed.count(), 0.1F);

        window_.pollEvents([&](const Event& event) {
            if (event.type == EventType::Closed) {
                window_.close();
                return;
            }
            inputFieldSystem_.handleEvent(registry_, event);
            buttonSystem_.handleEvent(registry_, event);
            menu.handleEvent(registry_, event);
        });

        window_.clear(Color(30, 30, 40));

        renderSystem_.update(registry_, dt);

        inputFieldSystem_.update(registry_, dt);
        buttonSystem_.update(registry_, dt);
        hudSystem_.update(registry_, dt);
        menu.render(registry_, window_);
        notificationSystem_.update(registry_, dt);
        window_.display();
    }
}
