#include "systems/RenderSystem.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ColliderComponent.hpp"
#include "components/HealthComponent.hpp"
#include "components/HitboxComponent.hpp"
#include "components/InvincibilityComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/abstraction/Common.hpp"

#include <algorithm>
#include <cmath>

RenderSystem::RenderSystem(Window& window) : window_(window) {}

void RenderSystem::update(Registry& registry, float deltaTime)
{
    struct DrawItem
    {
        int layer;
        enum class Type
        {
            Sprite,
            Box
        } type;
        SpriteComponent* spriteComp{nullptr};
        const BoxComponent* boxComp{nullptr};
        const TransformComponent* transform{nullptr};
        bool isVisible{true};
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
        auto sprite           = spriteComp.getSprite();

        if (registry.has<InvincibilityComponent>(id)) {
            auto& inv = registry.get<InvincibilityComponent>(id);
            inv.blinkTimer += deltaTime;
            if (inv.blinkTimer >= 0.1F) {
                inv.blinkTimer = 0.0F;
                inv.isVisible  = !inv.isVisible;
            }
            currentIsVisible = inv.isVisible;
            auto color       = sprite->getColor();
            color.a          = inv.isVisible ? 255 : 50;
            sprite->setColor(color);
        } else {
            auto color = sprite->getColor();
            if (color.a != 255) {
                color.a = 255;
                sprite->setColor(color);
            }
        }

        if (registry.has<HealthComponent>(id)) {
            if (registry.get<HealthComponent>(id).current <= 0) {
                currentIsVisible = false;
            }
        }

        if (registry.has<LayerComponent>(id)) {
            layer = registry.get<LayerComponent>(id).layer;
        }
        drawQueue.push_back(
            DrawItem{layer, DrawItem::Type::Sprite, &spriteComp, nullptr, &transform, currentIsVisible});
    }

    for (EntityId id : registry.view<TransformComponent, BoxComponent>()) {
        const auto& transform = registry.get<TransformComponent>(id);
        const auto& box       = registry.get<BoxComponent>(id);
        int layer             = RenderLayer::UI;
        if (registry.has<LayerComponent>(id)) {
            layer = registry.get<LayerComponent>(id).layer;
        }
        drawQueue.push_back(DrawItem{layer, DrawItem::Type::Box, nullptr, &box, &transform, box.visible});
    }

    auto typePriority = [](DrawItem::Type t) { return t == DrawItem::Type::Box ? 0 : 1; };

    std::stable_sort(drawQueue.begin(), drawQueue.end(), [&](const DrawItem& a, const DrawItem& b) {
        if (a.layer == b.layer)
            return typePriority(a.type) < typePriority(b.type);
        return a.layer < b.layer;
    });

    for (auto& item : drawQueue) {
        if (!item.isVisible)
            continue;

        if (item.type == DrawItem::Type::Sprite) {
            auto spritePtr = item.spriteComp->getSprite();
            if (!spritePtr) {
                continue;
            }
            ISprite& sprite = *spritePtr;
            sprite.setPosition(Vector2f{item.transform->x, item.transform->y});
            sprite.setScale(Vector2f{item.transform->scaleX, item.transform->scaleY});
            sprite.setRotation(item.transform->rotation);
            window_.draw(sprite);
        } else if (item.type == DrawItem::Type::Box) {
            const auto& box = *item.boxComp;
            window_.drawRectangle({box.width, box.height}, {item.transform->x, item.transform->y},
                                  item.transform->rotation, {item.transform->scaleX, item.transform->scaleY},
                                  box.fillColor, box.outlineColor, box.outlineThickness);
        }
    }
}
