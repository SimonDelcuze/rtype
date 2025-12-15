#include "ui/MenuRunner.hpp"

#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"

MenuRunner::MenuRunner(Window& window, FontManager& fonts, TextureManager& textures, std::atomic<bool>& running)
    : window_(window), fonts_(fonts), textures_(textures), running_(running), inputFieldSystem_(window, fonts),
      buttonSystem_(window, fonts), hudSystem_(window, fonts, textures)
{}

Registry& MenuRunner::getRegistry()
{
    return registry_;
}

void MenuRunner::runLoop(IMenu& menu)
{
    while (window_.isOpen() && !menu.isDone() && running_) {
        window_.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                window_.close();
                return;
            }
            inputFieldSystem_.handleEvent(registry_, event);
            buttonSystem_.handleEvent(registry_, event);
            menu.handleEvent(registry_, event);
        });

        window_.clear(sf::Color(30, 30, 40));

        for (EntityId entity : registry_.view<TransformComponent, SpriteComponent>()) {
            auto& transform = registry_.get<TransformComponent>(entity);
            auto& sprite    = registry_.get<SpriteComponent>(entity);
            if (sprite.hasSprite()) {
                auto* spr = const_cast<sf::Sprite*>(sprite.raw());
                spr->setPosition(sf::Vector2f{transform.x, transform.y});
                spr->setScale(sf::Vector2f{transform.scaleX, transform.scaleY});
                window_.draw(*spr);
            }
        }

        inputFieldSystem_.update(registry_, 0.0F);
        buttonSystem_.update(registry_, 0.0F);
        hudSystem_.update(registry_, 0.0F);
        menu.render(registry_, window_);
        window_.display();
    }
}
