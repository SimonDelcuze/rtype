#include "systems/HitboxDebugSystem.hpp"

#include "components/ColliderComponent.hpp"
#include "components/HitboxComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

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
        if (!registry.has<TransformComponent>(entity) || !registry.has<TagComponent>(entity)) {
            continue;
        }
        const auto& tag = registry.get<TagComponent>(entity);
        if (!tag.hasTag(EntityTag::Obstacle)) {
            continue;
        }
        const auto& transform = registry.get<TransformComponent>(entity);

        if (registry.has<ColliderComponent>(entity)) {
            const auto& col = registry.get<ColliderComponent>(entity);
            if (!col.isActive)
                continue;
            if (col.shape == ColliderComponent::Shape::Circle) {
                float scale = std::max(std::abs(transform.scaleX), std::abs(transform.scaleY));
                float r     = col.radius * scale;
                sf::CircleShape circle(r);
                circle.setPosition(sf::Vector2f{transform.x + col.offsetX - r, transform.y + col.offsetY - r});
                circle.setFillColor(sf::Color::Transparent);
                circle.setOutlineColor(color_);
                circle.setOutlineThickness(thickness_);
                window_.draw(circle);
            } else {
                sf::VertexArray poly;
                poly.setPrimitiveType(sf::PrimitiveType::LineStrip);
                for (const auto& p : col.points) {
                    float x = transform.x + (p[0] + col.offsetX) * transform.scaleX;
                    float y = transform.y + (p[1] + col.offsetY) * transform.scaleY;
                    poly.append(sf::Vertex{{x, y}, color_});
                }
                if (!col.points.empty()) {
                    float x = transform.x + (col.points.front()[0] + col.offsetX) * transform.scaleX;
                    float y = transform.y + (col.points.front()[1] + col.offsetY) * transform.scaleY;
                    poly.append(sf::Vertex{{x, y}, color_});
                }
                window_.draw(poly);
            }
            continue;
        }

        if (!registry.has<HitboxComponent>(entity)) {
            continue;
        }
        const auto& hitbox = registry.get<HitboxComponent>(entity);
        if (!hitbox.isActive) {
            continue;
        }
        float hitboxX = transform.x + hitbox.offsetX;
        float hitboxY = transform.y + hitbox.offsetY;

        sf::RectangleShape rect;
        rect.setPosition(sf::Vector2f{hitboxX, hitboxY});
        rect.setSize(sf::Vector2f{hitbox.width * transform.scaleX, hitbox.height * transform.scaleY});
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
