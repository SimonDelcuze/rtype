#include "systems/HitboxDebugSystem.hpp"

#include "components/ColliderComponent.hpp"
#include "components/HitboxComponent.hpp"
#include "components/TagComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/abstraction/Common.hpp"

#include <cmath>
#include <vector>
#include <algorithm>

HitboxDebugSystem::HitboxDebugSystem(Window& window) : window_(window) {}

void HitboxDebugSystem::update(Registry& registry)
{
    if (!enabled_) {
        return;
    }

    Color drawColor(color_.r, color_.g, color_.b, color_.a);

    for (EntityId entity = 0; entity < registry.entityCount(); ++entity) {
        if (!registry.isAlive(entity)) {
            continue;
        }
        if (!registry.has<TransformComponent>(entity) || !registry.has<TagComponent>(entity)) {
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
                float cx    = transform.x + col.offsetX;
                float cy    = transform.y + col.offsetY;
                
                const int segments = 32;
                std::vector<Vector2f> points;
                points.reserve(segments + 1);
                
                for (int i = 0; i <= segments; ++i) {
                    float theta = (static_cast<float>(i) / segments) * 6.28318530718f;
                    float px = cx + std::cos(theta) * r;
                    float py = cy + std::sin(theta) * r;
                    points.push_back({px, py});
                }
                
                window_.draw(points.data(), points.size(), drawColor, 2);
                
            } else {
                std::vector<Vector2f> points;
                for (const auto& p : col.points) {
                    float x = transform.x + (p[0] + col.offsetX) * transform.scaleX;
                    float y = transform.y + (p[1] + col.offsetY) * transform.scaleY;
                    points.push_back({x, y});
                }
                if (!points.empty()) {
                    points.push_back(points.front());
                }
                
                if (!points.empty()) {
                    window_.draw(points.data(), points.size(), drawColor, 2);
                }
            }
            continue;
        }

        if (registry.has<HitboxComponent>(entity)) {
            const auto& hitbox = registry.get<HitboxComponent>(entity);
            if (!hitbox.isActive) {
                continue;
            }
            
            float x = transform.x + hitbox.offsetX;
            float y = transform.y + hitbox.offsetY;
            float w = hitbox.width * transform.scaleX;
            float h = hitbox.height * transform.scaleY;
            
            Vector2f rect[5] = {
                {x, y},
                {x + w, y},
                {x + w, y + h},
                {x, y + h},
                {x, y}
            };
            
            window_.draw(rect, 5, drawColor, 2);
        }
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
