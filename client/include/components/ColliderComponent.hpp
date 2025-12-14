#pragma once

#include <array>
#include <vector>

struct ColliderComponent
{
    enum class Shape
    {
        Box,
        Circle,
        Polygon
    };

    Shape shape   = Shape::Box;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float width   = 0.0F;
    float height  = 0.0F;
    float radius  = 0.0F;
    bool isActive = true;
    std::vector<std::array<float, 2>> points;

    static ColliderComponent box(float width, float height, float offsetX = 0.0F, float offsetY = 0.0F,
                                 bool active = true);
    static ColliderComponent circle(float radius, float offsetX = 0.0F, float offsetY = 0.0F, bool active = true);
    static ColliderComponent polygon(const std::vector<std::array<float, 2>>& pts, float offsetX = 0.0F,
                                     float offsetY = 0.0F, bool active = true);
};

inline ColliderComponent ColliderComponent::box(float w, float h, float ox, float oy, bool active)
{
    ColliderComponent c;
    c.shape    = Shape::Box;
    c.width    = w;
    c.height   = h;
    c.offsetX  = ox;
    c.offsetY  = oy;
    c.isActive = active;
    return c;
}

inline ColliderComponent ColliderComponent::circle(float r, float ox, float oy, bool active)
{
    ColliderComponent c;
    c.shape    = Shape::Circle;
    c.radius   = r;
    c.offsetX  = ox;
    c.offsetY  = oy;
    c.isActive = active;
    return c;
}

inline ColliderComponent ColliderComponent::polygon(const std::vector<std::array<float, 2>>& pts, float ox, float oy,
                                                    bool active)
{
    ColliderComponent c;
    c.shape    = Shape::Polygon;
    c.points   = pts;
    c.offsetX  = ox;
    c.offsetY  = oy;
    c.isActive = active;
    return c;
}
