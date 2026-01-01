#include "systems/RenderSystem.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ColliderComponent.hpp"
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
        SpriteComponent* spriteComp;
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

        if (registry.has<LayerComponent>(id)) {
            layer = registry.get<LayerComponent>(id).layer;
        }
        drawQueue.push_back(DrawItem{layer, &spriteComp, &transform, currentIsVisible});
    }

    std::stable_sort(drawQueue.begin(), drawQueue.end(),
                     [](const DrawItem& a, const DrawItem& b) { return a.layer < b.layer; });

    for (auto& item : drawQueue) {
        if (!item.isVisible)
            continue;

        auto spritePtr = item.spriteComp->getSprite();
        if (!spritePtr) {
            continue;
        }
        ISprite& sprite = *spritePtr;
        sprite.setPosition(Vector2f{item.transform->x, item.transform->y});
        sprite.setScale(Vector2f{item.transform->scaleX, item.transform->scaleY});
        sprite.setRotation(item.transform->rotation);
        window_.draw(sprite);
    }

    for (EntityId id : registry.view<TransformComponent, BoxComponent>()) {
        const auto& transform = registry.get<TransformComponent>(id);
        const auto& box       = registry.get<BoxComponent>(id);

        float w = box.width;
        float h = box.height;

        Vector2f loc[4] = {{0.0f, 0.0f}, {0.0f, h}, {w, 0.0f}, {w, h}};

        float rad = transform.rotation * 3.14159265359f / 180.0f;
        float c   = std::cos(rad);
        float s   = std::sin(rad);

        Vector2f world[4];
        for (int i = 0; i < 4; ++i) {
            float sx = loc[i].x * transform.scaleX;
            float sy = loc[i].y * transform.scaleY;

            float rx = sx * c - sy * s;
            float ry = sx * s + sy * c;

            world[i].x = rx + transform.x;
            world[i].y = ry + transform.y;
        }

        if (box.fillColor.a > 0) {
            window_.draw(world, 4, box.fillColor, 4);
        }

        if (box.outlineThickness > 0.0f && box.outlineColor.a > 0) {
            Vector2f outline[5] = {world[0], world[2], world[3], world[1], world[0]};
            window_.draw(outline, 5, box.outlineColor, 2);
        }
    }
}
