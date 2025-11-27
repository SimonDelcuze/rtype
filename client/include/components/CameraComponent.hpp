#pragma once

#include <cstdint>
#include <limits>

using EntityId = std::uint32_t;

struct CameraComponent
{
    float x        = 0.0F;
    float y        = 0.0F;
    float zoom     = 1.0F;
    float offsetX  = 0.0F;
    float offsetY  = 0.0F;
    float rotation = 0.0F;
    bool active    = true;

    EntityId targetEntity  = std::numeric_limits<EntityId>::max();
    float followSmoothness = 5.0F;
    bool followEnabled     = false;

    static CameraComponent create(float x, float y, float zoom = 1.0F);

    void setPosition(float newX, float newY);

    void move(float dx, float dy);

    void setZoom(float newZoom);

    void setOffset(float newOffsetX, float newOffsetY);

    void setRotation(float degrees);

    void rotate(float degrees);

    void reset();

    void clampZoom(float minZoom, float maxZoom);

    void setTarget(EntityId entity, float smoothness = 5.0F);

    void clearTarget();
};

inline CameraComponent CameraComponent::create(float x, float y, float zoom)
{
    CameraComponent camera;
    camera.x    = x;
    camera.y    = y;
    camera.zoom = zoom;
    return camera;
}

inline void CameraComponent::setPosition(float newX, float newY)
{
    x = newX;
    y = newY;
}

inline void CameraComponent::move(float dx, float dy)
{
    x += dx;
    y += dy;
}

inline void CameraComponent::setZoom(float newZoom)
{
    if (newZoom > 0.0F) {
        zoom = newZoom;
    }
}

inline void CameraComponent::setOffset(float newOffsetX, float newOffsetY)
{
    offsetX = newOffsetX;
    offsetY = newOffsetY;
}

inline void CameraComponent::setRotation(float degrees)
{
    rotation = degrees;
}

inline void CameraComponent::rotate(float degrees)
{
    rotation += degrees;
}

inline void CameraComponent::reset()
{
    x                = 0.0F;
    y                = 0.0F;
    zoom             = 1.0F;
    offsetX          = 0.0F;
    offsetY          = 0.0F;
    rotation         = 0.0F;
    targetEntity     = std::numeric_limits<EntityId>::max();
    followSmoothness = 5.0F;
    followEnabled    = false;
}

inline void CameraComponent::clampZoom(float minZoom, float maxZoom)
{
    if (zoom < minZoom) {
        zoom = minZoom;
    } else if (zoom > maxZoom) {
        zoom = maxZoom;
    }
}

inline void CameraComponent::setTarget(EntityId entity, float smoothness)
{
    targetEntity     = entity;
    followSmoothness = smoothness;
    followEnabled    = true;
}

inline void CameraComponent::clearTarget()
{
    targetEntity  = std::numeric_limits<EntityId>::max();
    followEnabled = false;
}
