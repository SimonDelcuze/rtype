#pragma once

struct CameraComponent
{
    float x      = 0.0F;
    float y      = 0.0F;
    float zoom   = 1.0F;
    float offsetX = 0.0F;
    float offsetY = 0.0F;
    float rotation = 0.0F;
    bool active   = true;

    static CameraComponent create(float x, float y, float zoom = 1.0F);

    void setPosition(float newX, float newY);

    void move(float dx, float dy);

    void setZoom(float newZoom);

    void setOffset(float newOffsetX, float newOffsetY);

    void setRotation(float degrees);

    void rotate(float degrees);

    void reset();

    void clampZoom(float minZoom, float maxZoom);
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
    x        = 0.0F;
    y        = 0.0F;
    zoom     = 1.0F;
    offsetX  = 0.0F;
    offsetY  = 0.0F;
    rotation = 0.0F;
}

inline void CameraComponent::clampZoom(float minZoom, float maxZoom)
{
    if (zoom < minZoom) {
        zoom = minZoom;
    } else if (zoom > maxZoom) {
        zoom = maxZoom;
    }
}
