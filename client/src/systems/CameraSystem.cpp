#include "systems/CameraSystem.hpp"

#include <algorithm>
#include <cmath>

CameraSystem::CameraSystem(sf::RenderWindow& window) : window_(window)
{
    view_ = window_.getView();
    auto windowSize = window_.getSize();
    baseViewSize_   = sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    view_.setSize(baseViewSize_);
}

void CameraSystem::update(Registry& registry)
{
    EntityId cameraId = findActiveCamera(registry);
    if (cameraId == 0) {
        return;
    }
    activeCameraId_ = cameraId;
    if (!registry.has<CameraComponent>(cameraId)) {
        return;
    }
    if (worldBoundsEnabled_) {
        auto& camera = registry.get<CameraComponent>(cameraId);
        clampToWorldBounds(camera);
    }
    const auto& camera = registry.get<CameraComponent>(cameraId);
    applyCamera(camera);
    window_.setView(view_);
}

sf::View& CameraSystem::getView()
{
    return view_;
}

const sf::View& CameraSystem::getView() const
{
    return view_;
}

void CameraSystem::setWorldBounds(float left, float top, float width, float height)
{
    worldLeft_   = left;
    worldTop_    = top;
    worldWidth_  = width;
    worldHeight_ = height;
}

void CameraSystem::setWorldBoundsEnabled(bool enabled)
{
    worldBoundsEnabled_ = enabled;
}

EntityId CameraSystem::getActiveCamera() const
{
    return activeCameraId_;
}

EntityId CameraSystem::findActiveCamera(Registry& registry)
{
    auto cameras = registry.view<CameraComponent>();

    for (EntityId entity : cameras) {
        if (registry.isAlive(entity)) {
            const auto& camera = registry.get<CameraComponent>(entity);
            if (camera.active) {
                return entity;
            }
        }
    }
    return 0;
}

void CameraSystem::applyCamera(const CameraComponent& camera)
{
    float centerX = camera.x + camera.offsetX;
    float centerY = camera.y + camera.offsetY;
    float zoomedWidth  = baseViewSize_.x / camera.zoom;
    float zoomedHeight = baseViewSize_.y / camera.zoom;
    view_.setCenter(sf::Vector2f(centerX, centerY));
    view_.setSize(sf::Vector2f(zoomedWidth, zoomedHeight));
    view_.setRotation(sf::degrees(camera.rotation));
}

void CameraSystem::clampToWorldBounds(CameraComponent& camera)
{
    float halfViewWidth  = (baseViewSize_.x / camera.zoom) / 2.0F;
    float halfViewHeight = (baseViewSize_.y / camera.zoom) / 2.0F;
    float minX = worldLeft_ + halfViewWidth;
    float maxX = worldLeft_ + worldWidth_ - halfViewWidth;
    if (maxX < minX) {
        camera.x = worldLeft_ + worldWidth_ / 2.0F;
    } else {
        camera.x = std::clamp(camera.x, minX, maxX);
    }
    float minY = worldTop_ + halfViewHeight;
    float maxY = worldTop_ + worldHeight_ - halfViewHeight;
    if (maxY < minY) {
        camera.y = worldTop_ + worldHeight_ / 2.0F;
    } else {
        camera.y = std::clamp(camera.y, minY, maxY);
    }
}
