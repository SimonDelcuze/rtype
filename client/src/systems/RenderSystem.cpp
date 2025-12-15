#include "systems/RenderSystem.hpp"

#include "components/InvincibilityComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>

RenderSystem::RenderSystem(Window& window) : window_(window) {}

void RenderSystem::update(Registry& registry, float deltaTime)
{
    struct DrawItem
    {
        int layer;
        SpriteComponent* sprite;
        TransformComponent* transform;
        bool isVisible;
    };

    std::vector<DrawItem> drawQueue;
    drawQueue.reserve(registry.entityCount());

    for (EntityId id : registry.view<TransformComponent, SpriteComponent>()) {
        auto& spriteComp = registry.get<SpriteComponent>(id);
        if (!spriteComp.hasSprite()) {
            continue;
        }
        auto& transform       = registry.get<TransformComponent>(id);
        int layer             = 0;
        bool currentIsVisible = true;

        if (registry.has<InvincibilityComponent>(id)) {
            auto& inv = registry.get<InvincibilityComponent>(id);
            inv.blinkTimer += deltaTime;
            if (inv.blinkTimer >= 0.1F) {
                inv.blinkTimer = 0.0F;
                inv.isVisible  = !inv.isVisible;
            }
            currentIsVisible = inv.isVisible;
            auto color       = spriteComp.sprite->getColor();
            color.a          = inv.isVisible ? 255 : 50;
            spriteComp.sprite->setColor(color);
        } else {
            auto color = spriteComp.sprite->getColor();
            if (color.a != 255) {
                color.a = 255;
                spriteComp.sprite->setColor(color);
            }
        }

        if (registry.has<LayerComponent>(id)) {
            layer = registry.get<LayerComponent>(id).layer;
        }
        drawQueue.push_back(DrawItem{layer, &spriteComp, &transform, currentIsVisible});
    }

    std::stable_sort(drawQueue.begin(), drawQueue.end(),
                     [](const DrawItem& a, const DrawItem& b) { return a.layer < b.layer; });

    for (auto& item : drawQueue) {
        auto* raw = item.sprite->raw();
        if (raw == nullptr) {
            continue;
        }
        auto& sprite = *item.sprite->sprite;
        sprite.setPosition(sf::Vector2f{item.transform->x, item.transform->y});
        sprite.setScale(sf::Vector2f{item.transform->scaleX, item.transform->scaleY});
        sprite.setRotation(sf::degrees(item.transform->rotation));
        window_.draw(sprite);
    }
}
