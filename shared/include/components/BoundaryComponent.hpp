#pragma once

struct BoundaryComponent
{
    float minX = 0.0F;
    float minY = 0.0F;
    float maxX = 1280.0F;
    float maxY = 720.0F;

    static BoundaryComponent create(float minX, float minY, float maxX, float maxY);
};

inline BoundaryComponent BoundaryComponent::create(float minX, float minY, float maxX, float maxY)
{
    BoundaryComponent component;
    component.minX = minX;
    component.minY = minY;
    component.maxX = maxX;
    component.maxY = maxY;
    return component;
}
