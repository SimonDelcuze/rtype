#include "systems/RenderSystem.hpp"

#include "components/InvincibilityComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "components/ColliderComponent.hpp"
#include "components/HitboxComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/TagComponent.hpp"

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

    constexpr bool kDebugColliders = false;
    if (!kDebugColliders) {
        return;
    }
    const sf::Color debugColor = sf::Color::Red;
    for (EntityId id : registry.view<TransformComponent, TagComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Obstacle))
            continue;
        const auto& t = registry.get<TransformComponent>(id);
        if (registry.has<ColliderComponent>(id)) {
            const auto& col = registry.get<ColliderComponent>(id);
            if (!col.isActive)
                continue;
            if (col.shape == ColliderComponent::Shape::Circle) {
                float scale   = std::max(std::abs(t.scaleX), std::abs(t.scaleY));
                float radius  = col.radius * scale;
                float centerX = t.x + col.offsetX;
                float centerY = t.y + col.offsetY;
                sf::CircleShape circle(radius);
                circle.setPosition(sf::Vector2f{centerX - radius, centerY - radius});
                circle.setFillColor(sf::Color::Transparent);
                circle.setOutlineColor(debugColor);
                circle.setOutlineThickness(1.0F);
                window_.draw(circle);
            } else {
                sf::VertexArray poly;
                poly.setPrimitiveType(sf::PrimitiveType::LineStrip);
                for (const auto& p : col.points) {
                    float x = t.x + (p[0] + col.offsetX) * t.scaleX;
                    float y = t.y + (p[1] + col.offsetY) * t.scaleY;
                    poly.append(sf::Vertex{{x, y}, debugColor});
                }
                if (!col.points.empty()) {
                    float x = t.x + (col.points.front()[0] + col.offsetX) * t.scaleX;
                    float y = t.y + (col.points.front()[1] + col.offsetY) * t.scaleY;
                    poly.append(sf::Vertex{{x, y}, debugColor});
                }
                window_.draw(poly);
            }
            continue;
        }
        if (registry.has<HitboxComponent>(id)) {
            const auto& h = registry.get<HitboxComponent>(id);
            if (!h.isActive)
                continue;
            sf::RectangleShape rect;
            rect.setPosition(sf::Vector2f{t.x + h.offsetX, t.y + h.offsetY});
            rect.setSize(sf::Vector2f{h.width * t.scaleX, h.height * t.scaleY});
            rect.setFillColor(sf::Color::Transparent);
            rect.setOutlineColor(debugColor);
            rect.setOutlineThickness(1.0F);
            window_.draw(rect);
        }
    }
}
