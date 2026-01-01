#include "ui/MenuRunner.hpp"

#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"

MenuRunner::MenuRunner(Window& window, FontManager& fonts, TextureManager& textures, std::atomic<bool>& running)
    : window_(window), fonts_(fonts), textures_(textures), running_(running), renderSystem_(window), inputFieldSystem_(window, fonts),
      buttonSystem_(window, fonts), hudSystem_(window, fonts, textures)
{}

Registry& MenuRunner::getRegistry()
{
    return registry_;
}

void MenuRunner::runLoop(IMenu& menu)
{
    while (window_.isOpen() && !menu.isDone() && running_) {
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

        renderSystem_.update(registry_, 0.0F);

        inputFieldSystem_.update(registry_, 0.0F);
        buttonSystem_.update(registry_, 0.0F);
        hudSystem_.update(registry_, 0.0F);
        menu.render(registry_, window_);
        window_.display();
    }
}
