#include "systems/HitboxDebugSystem.hpp"

#include "components/HitboxComponent.hpp"
#include "components/TransformComponent.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

HitboxDebugSystem::HitboxDebugSystem(Window& window) : window_(window) {}

void HitboxDebugSystem::update(Registry& registry)
{
    if (!enabled_) {
        return;
    }

    for (EntityId entity = 0; entity < registry.entityCount(); ++entity) {
        if (!registry.isAlive(entity)) {
            continue;
        }

        if (!registry.has<HitboxComponent>(entity) || !registry.has<TransformComponent>(entity)) {
            continue;
        }

        const auto& hitbox    = registry.get<HitboxComponent>(entity);
        const auto& transform = registry.get<TransformComponent>(entity);

        if (!hitbox.isActive) {
            continue;
        }

        float hitboxX = transform.x + hitbox.offsetX - hitbox.width / 2.0F;
        float hitboxY = transform.y + hitbox.offsetY - hitbox.height / 2.0F;

        sf::RectangleShape rect;
        rect.setPosition(sf::Vector2f{hitboxX, hitboxY});
        rect.setSize(sf::Vector2f{hitbox.width, hitbox.height});
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(color_);
        rect.setOutlineThickness(thickness_);

        window_.draw(rect);
    }
}

void HitboxDebugSystem::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

void HitboxDebugSystem::setColor(const sf::Color& color)
{
    color_ = color;
}

void HitboxDebugSystem::setThickness(float thickness)
{
    thickness_ = thickness;
}
