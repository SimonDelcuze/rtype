#include "systems/CollisionSystem.hpp"

#include <cmath>
#include <limits>
#include <optional>

namespace
{
    struct Vec2
    {
        float x = 0.0F;
        float y = 0.0F;
    };

    struct Shape
    {
        ColliderComponent::Shape type = ColliderComponent::Shape::Box;
        bool active                   = false;
        std::vector<Vec2> points;
        Vec2 center{};
        float radius = 0.0F;
        std::array<float, 4> aabb{};
    };

    bool finite(float v)
    {
        return std::isfinite(v);
    }

    bool normalize(Vec2& v)
    {
        float len = std::sqrt(v.x * v.x + v.y * v.y);
        if (len <= 0.0F || !std::isfinite(len))
            return false;
        v.x /= len;
        v.y /= len;
        return true;
    }

    std::array<float, 2> projectPolygon(const std::vector<Vec2>& pts, const Vec2& axis)
    {
        float min = std::numeric_limits<float>::infinity();
        float max = -std::numeric_limits<float>::infinity();
        for (const auto& p : pts) {
            float proj = p.x * axis.x + p.y * axis.y;
            min        = std::min(min, proj);
            max        = std::max(max, proj);
        }
        return {min, max};
    }

    std::array<float, 2> projectCircle(const Vec2& center, float radius, const Vec2& axis)
    {
        float proj = center.x * axis.x + center.y * axis.y;
        return {proj - radius, proj + radius};
    }

    bool rangesOverlap(const std::array<float, 2>& a, const std::array<float, 2>& b)
    {
        return !(a[1] < b[0] || b[1] < a[0]);
    }

    bool polygonPolygon(const std::vector<Vec2>& a, const std::vector<Vec2>& b)
    {
        auto checkAxes = [&](const std::vector<Vec2>& poly1, const std::vector<Vec2>& poly2) {
            for (std::size_t i = 0; i < poly1.size(); ++i) {
                const auto& p1 = poly1[i];
                const auto& p2 = poly1[(i + 1) % poly1.size()];
                Vec2 edge{p2.x - p1.x, p2.y - p1.y};
                Vec2 axis{-edge.y, edge.x};
                if (!normalize(axis))
                    continue;
                auto projA = projectPolygon(poly1, axis);
                auto projB = projectPolygon(poly2, axis);
                if (!rangesOverlap(projA, projB))
                    return false;
            }
            return true;
        };

        return checkAxes(a, b) && checkAxes(b, a);
    }

    float distanceSquared(const Vec2& a, const Vec2& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return dx * dx + dy * dy;
    }

    bool circleCircle(const Shape& a, const Shape& b)
    {
        float r = a.radius + b.radius;
        return distanceSquared(a.center, b.center) <= r * r;
    }

    bool circlePolygon(const Shape& circle, const Shape& poly)
    {
        if (poly.points.empty())
            return false;
        float bestDist = std::numeric_limits<float>::infinity();
        Vec2 closest{};
        for (const auto& p : poly.points) {
            float d = distanceSquared(circle.center, p);
            if (d < bestDist) {
                bestDist = d;
                closest  = p;
            }
        }

        std::vector<Vec2> axes;
        axes.reserve(poly.points.size() + 1);
        for (std::size_t i = 0; i < poly.points.size(); ++i) {
            const auto& p1 = poly.points[i];
            const auto& p2 = poly.points[(i + 1) % poly.points.size()];
            Vec2 edge{p2.x - p1.x, p2.y - p1.y};
            Vec2 axis{-edge.y, edge.x};
            if (normalize(axis))
                axes.push_back(axis);
        }
        Vec2 axisToVertex{closest.x - circle.center.x, closest.y - circle.center.y};
        if (normalize(axisToVertex))
            axes.push_back(axisToVertex);

        for (const auto& axis : axes) {
            auto projCircle = projectCircle(circle.center, circle.radius, axis);
            auto projPoly   = projectPolygon(poly.points, axis);
            if (!rangesOverlap(projCircle, projPoly))
                return false;
        }
        return true;
    }

    std::optional<Shape> buildShape(const TransformComponent& t, const ColliderComponent* collider,
                                    const HitboxComponent* hitbox)
    {
        Shape s{};
        const bool hasCollider = collider != nullptr;
        const bool hasHitbox   = hitbox != nullptr;
        if (!hasCollider && !hasHitbox)
            return std::nullopt;
        if (hasCollider) {
            s.active = collider->isActive;
            s.type   = collider->shape;
        } else {
            s.active = hitbox->isActive;
            s.type   = ColliderComponent::Shape::Box;
        }
        if (!s.active)
            return std::nullopt;
        if (!finite(t.x) || !finite(t.y) || !finite(t.scaleX) || !finite(t.scaleY))
            return std::nullopt;

        const float sx = t.scaleX;
        const float sy = t.scaleY;

        auto toWorld = [&](float px, float py, float ox, float oy) -> Vec2 {
            return Vec2{t.x + (px + ox) * sx, t.y + (py + oy) * sy};
        };

        if (s.type == ColliderComponent::Shape::Box) {
            float w  = hasCollider ? collider->width : hitbox->width;
            float h  = hasCollider ? collider->height : hitbox->height;
            float ox = hasCollider ? collider->offsetX : hitbox->offsetX;
            float oy = hasCollider ? collider->offsetY : hitbox->offsetY;
            if (w <= 0.0F || h <= 0.0F || !finite(w) || !finite(h) || !finite(ox) || !finite(oy))
                return std::nullopt;
            s.points.push_back(toWorld(0.0F, 0.0F, ox, oy));
            s.points.push_back(toWorld(w, 0.0F, ox, oy));
            s.points.push_back(toWorld(w, h, ox, oy));
            s.points.push_back(toWorld(0.0F, h, ox, oy));
        } else if (s.type == ColliderComponent::Shape::Circle) {
            float r  = hasCollider ? collider->radius : 0.0F;
            float ox = hasCollider ? collider->offsetX : 0.0F;
            float oy = hasCollider ? collider->offsetY : 0.0F;
            if (r <= 0.0F || !finite(r) || !finite(ox) || !finite(oy))
                return std::nullopt;
            float scaleFactor = std::max(std::abs(sx), std::abs(sy));
            s.radius          = r * scaleFactor;
            s.center          = toWorld(0.0F, 0.0F, ox, oy);
        } else if (s.type == ColliderComponent::Shape::Polygon) {
            if (collider == nullptr || collider->points.empty())
                return std::nullopt;
            float ox = collider->offsetX;
            float oy = collider->offsetY;
            if (!finite(ox) || !finite(oy))
                return std::nullopt;
            s.points.reserve(collider->points.size());
            for (const auto& p : collider->points) {
                if (!finite(p[0]) || !finite(p[1]))
                    return std::nullopt;
                s.points.push_back(toWorld(p[0], p[1], ox, oy));
            }
        }

        if (s.type != ColliderComponent::Shape::Circle) {
            if (s.points.size() < 3)
                return std::nullopt;
            float minX = std::numeric_limits<float>::infinity();
            float maxX = -std::numeric_limits<float>::infinity();
            float minY = std::numeric_limits<float>::infinity();
            float maxY = -std::numeric_limits<float>::infinity();
            for (const auto& p : s.points) {
                minX = std::min(minX, p.x);
                maxX = std::max(maxX, p.x);
                minY = std::min(minY, p.y);
                maxY = std::max(maxY, p.y);
            }
            s.aabb = {minX, maxX, minY, maxY};
        } else {
            float minX = s.center.x - s.radius;
            float maxX = s.center.x + s.radius;
            float minY = s.center.y - s.radius;
            float maxY = s.center.y + s.radius;
            s.aabb     = {minX, maxX, minY, maxY};
        }
        return s;
    }

    bool aabbOverlap(const std::array<float, 4>& a, const std::array<float, 4>& b)
    {
        return !(a[1] < b[0] || a[0] > b[1] || a[3] < b[2] || a[2] > b[3]);
    }

    bool intersect(const Shape& a, const Shape& b)
    {
        if (!a.active || !b.active)
            return false;
        if (!aabbOverlap(a.aabb, b.aabb))
            return false;

        if (a.type == ColliderComponent::Shape::Circle && b.type == ColliderComponent::Shape::Circle)
            return circleCircle(a, b);
        if (a.type == ColliderComponent::Shape::Circle && b.type == ColliderComponent::Shape::Polygon)
            return circlePolygon(a, b);
        if (a.type == ColliderComponent::Shape::Polygon && b.type == ColliderComponent::Shape::Circle)
            return circlePolygon(b, a);
        if (a.type == ColliderComponent::Shape::Polygon && b.type == ColliderComponent::Shape::Polygon)
            return polygonPolygon(a.points, b.points);
        if (a.type == ColliderComponent::Shape::Circle && b.type == ColliderComponent::Shape::Box) {
            Shape polyB = b;
            polyB.type  = ColliderComponent::Shape::Polygon;
            return circlePolygon(a, polyB);
        }
        if (a.type == ColliderComponent::Shape::Box && b.type == ColliderComponent::Shape::Circle) {
            Shape polyA = a;
            polyA.type  = ColliderComponent::Shape::Polygon;
            return circlePolygon(b, polyA);
        }
        // Box vs box or box vs polygon handled as polygons
        Shape polyA = a;
        Shape polyB = b;
        polyA.type  = ColliderComponent::Shape::Polygon;
        polyB.type  = ColliderComponent::Shape::Polygon;
        return polygonPolygon(polyA.points, polyB.points);
    }
} // namespace

std::vector<Collision> CollisionSystem::detect(Registry& registry) const
{
    std::vector<EntityId> ids;
    std::vector<Shape> shapes;
    ids.reserve(registry.entityCount());
    shapes.reserve(registry.entityCount());

    for (EntityId id : registry.view<TransformComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const TransformComponent& t = registry.get<TransformComponent>(id);
        const ColliderComponent* col =
            registry.has<ColliderComponent>(id) ? &registry.get<ColliderComponent>(id) : nullptr;
        const HitboxComponent* hb = registry.has<HitboxComponent>(id) ? &registry.get<HitboxComponent>(id) : nullptr;
        auto shape                = buildShape(t, col, hb);
        if (!shape)
            continue;
        ids.push_back(id);
        shapes.push_back(*shape);
    }

    std::vector<Collision> out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        for (std::size_t j = i + 1; j < ids.size(); ++j) {
            if (intersect(shapes[i], shapes[j])) {
                out.push_back(Collision{ids[i], ids[j]});
            }
        }
    }
    return out;
}
