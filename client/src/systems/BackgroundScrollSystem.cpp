#include "systems/BackgroundScrollSystem.hpp"
#include "graphics/GraphicsFactory.hpp"

#include <algorithm>
#include <limits>

BackgroundScrollSystem::BackgroundScrollSystem(Window& window) : window_(window) {}

void BackgroundScrollSystem::setNextBackground(const std::shared_ptr<ITexture>& texture)
{
    nextTexture_ = texture;
}

void BackgroundScrollSystem::collectEntries(Registry& registry, std::vector<Entry>& entries) const
{
    for (EntityId entity : registry.view<TransformComponent, BackgroundScrollComponent, SpriteComponent>()) {
        if (!registry.isAlive(entity)) {
            continue;
        }
        auto& transform = registry.get<TransformComponent>(entity);
        auto& scroll    = registry.get<BackgroundScrollComponent>(entity);
        auto& sprite    = registry.get<SpriteComponent>(entity);
        if (!sprite.sprite) {
            continue;
        }
        
        auto tex = sprite.texture;
        if (!tex) continue;

        int layer = 0;
        if (registry.has<LayerComponent>(entity)) {
            layer = registry.get<LayerComponent>(entity).layer;
        }
        entries.push_back(Entry{entity, &transform, &scroll, &sprite, 0.0F, 0.0F, layer, tex});
    }
}

void BackgroundScrollSystem::applyScaleAndOffsets(std::vector<Entry>& entries, float windowHeight) const
{
    for (auto& entry : entries) {
        if (!entry.texture) {
            continue;
        }
        const auto texSize = entry.texture->getSize();
        float scale        = 1.0F;
        if (texSize.y != 0) {
            scale                   = windowHeight / static_cast<float>(texSize.y);
            entry.transform->scaleX = scale;
            entry.transform->scaleY = scale;
        }
        if (entry.scroll->resetOffsetX == 0.0F) {
            entry.scroll->resetOffsetX = static_cast<float>(texSize.x) * scale;
        }
        if (entry.scroll->resetOffsetY == 0.0F) {
            entry.scroll->resetOffsetY = static_cast<float>(texSize.y) * scale;
        }
        entry.width  = entry.scroll->resetOffsetX;
        entry.height = entry.scroll->resetOffsetY;
    }
}

void BackgroundScrollSystem::moveAndTrack(std::vector<Entry>& entries, float deltaTime, float& minX, float& maxX) const
{
    for (auto& entry : entries) {
        entry.transform->x += entry.scroll->speedX * deltaTime;
        entry.transform->y += entry.scroll->speedY * deltaTime;
        minX = std::min(minX, entry.transform->x);
        maxX = std::max(maxX, entry.transform->x);
    }
}

void BackgroundScrollSystem::ensureCoverage(Registry& registry, std::vector<Entry>& entries, float windowWidth) const
{
    if (entries.empty()) {
        return;
    }
    float minX                 = std::numeric_limits<float>::infinity();
    float maxX                 = -std::numeric_limits<float>::infinity();
    float baseWidth            = entries.front().width;
    float baseHeight           = entries.front().height;
    std::shared_ptr<ITexture> baseTex = entries.front().texture;
    int baseLayer              = entries.front().layer;
    const float baseSpeedX     = entries.front().scroll->speedX;
    const float baseSpeedY     = entries.front().scroll->speedY;
    const float baseScaleX     = entries.front().transform->scaleX;
    const float baseScaleY     = entries.front().transform->scaleY;
    const float baseY          = entries.front().transform->y;
    for (const auto& entry : entries) {
        minX       = std::min(minX, entry.transform->x);
        maxX       = std::max(maxX, entry.transform->x);
        baseWidth  = std::max(baseWidth, entry.width);
        baseHeight = std::max(baseHeight, entry.height);
    }
    if (baseWidth <= 0.0F || !baseTex) {
        return;
    }
    float coverage = (maxX + baseWidth) - minX;
    while (coverage < windowWidth + baseWidth) {
        EntityId e = registry.createEntity();
        registry.emplace<SpriteComponent>(e, SpriteComponent(baseTex));
        registry.emplace<TransformComponent>(e, TransformComponent::create(maxX + baseWidth, baseY));
        registry.emplace<BackgroundScrollComponent>(
            e, BackgroundScrollComponent::create(baseSpeedX, baseSpeedY, baseWidth, baseHeight));
        if (baseLayer != 0) {
            registry.emplace<LayerComponent>(e, LayerComponent::create(baseLayer));
        }
        auto& t  = registry.get<TransformComponent>(e);
        auto& sc = registry.get<BackgroundScrollComponent>(e);
        auto& sp = registry.get<SpriteComponent>(e);
        t.scaleX = baseScaleX;
        t.scaleY = baseScaleY;
        entries.push_back(Entry{e, &t, &sc, &sp, baseWidth, baseHeight, baseLayer, baseTex});
        maxX     = t.x;
        coverage = (maxX + baseWidth) - minX;
    }
}

void BackgroundScrollSystem::applyTextureChange(Entry& entry, const std::shared_ptr<ITexture>& texture, float windowHeight) const
{
    if (entry.sprite && entry.sprite->sprite && texture) {
        entry.sprite->sprite->setTexture(*texture);
        entry.sprite->texture = texture;
    }
    entry.texture      = texture;
    const auto texSize = texture->getSize();
    float scale        = 1.0F;
    if (texSize.y != 0) {
        scale                   = windowHeight / static_cast<float>(texSize.y);
        entry.transform->scaleX = scale;
        entry.transform->scaleY = scale;
    }
    entry.scroll->resetOffsetX = static_cast<float>(texSize.x) * scale;
    entry.scroll->resetOffsetY = static_cast<float>(texSize.y) * scale;
    entry.width                = entry.scroll->resetOffsetX;
    entry.height               = entry.scroll->resetOffsetY;
}

void BackgroundScrollSystem::wrapAndSwap(std::vector<Entry>& entries, float windowHeight)
{
    float maxX = -std::numeric_limits<float>::infinity();
    for (auto& entry : entries) {
        if (entry.transform->x > maxX) {
            maxX = entry.transform->x;
        }
    }
    for (auto& entry : entries) {
        if (entry.scroll->resetOffsetX > 0.0F && entry.transform->x <= -entry.scroll->resetOffsetX) {
            if (nextTexture_ != nullptr) {
                applyTextureChange(entry, nextTexture_, windowHeight);
            }
            entry.transform->x = maxX + entry.scroll->resetOffsetX;
            maxX               = entry.transform->x;
        }
        if (entry.scroll->resetOffsetY > 0.0F && entry.transform->y <= -entry.scroll->resetOffsetY) {
            entry.transform->y += entry.scroll->resetOffsetY;
        }
    }
    nextTexture_ = nullptr;
}

void BackgroundScrollSystem::update(Registry& registry, float deltaTime)
{
    const float windowHeight = static_cast<float>(window_.getSize().y);
    const float windowWidth  = static_cast<float>(window_.getSize().x);
    std::vector<Entry> entries;
    entries.reserve(registry.entityCount());
    collectEntries(registry, entries);
    if (entries.empty()) {
        return;
    }
    applyScaleAndOffsets(entries, windowHeight);
    ensureCoverage(registry, entries, windowWidth);

    entries.clear();
    collectEntries(registry, entries);

    float minX = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    moveAndTrack(entries, deltaTime, minX, maxX);
    wrapAndSwap(entries, windowHeight);
}
