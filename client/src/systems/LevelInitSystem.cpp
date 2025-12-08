#include "systems/LevelInitSystem.hpp"

#include "components/BackgroundScrollComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"

#include <iostream>

LevelInitSystem::LevelInitSystem(ThreadSafeQueue<LevelInitData>& queue, EntityTypeRegistry& typeRegistry,
                                 const AssetManifest& manifest, TextureManager& textures, AnimationRegistry& animations,
                                 AnimationLabels& labels, LevelState& state)
    : queue_(&queue), typeRegistry_(&typeRegistry), manifest_(&manifest), textures_(&textures),
      animations_(&animations), labels_(&labels), state_(&state)
{}

void LevelInitSystem::initialize() {}

void LevelInitSystem::update(Registry& registry, float)
{
    LevelInitData data;
    while (queue_->tryPop(data)) {
        processLevelInit(registry, data);
    }
}

void LevelInitSystem::cleanup() {}

void LevelInitSystem::processLevelInit(Registry& registry, const LevelInitData& data)
{
    typeRegistry_->clear();
    state_->levelId = data.levelId;
    state_->seed    = data.seed;
    state_->active  = true;
    applyBackground(registry, data);
    for (const auto& entry : data.archetypes) {
        resolveEntityType(entry);
    }
}

void LevelInitSystem::resolveEntityType(const ArchetypeEntry& entry)
{
    RenderTypeData renderData = buildRenderData(entry);
    if (renderData.texture == nullptr ||
        (entry.animId.size() && renderData.animation == nullptr && animations_ != nullptr && labels_ != nullptr)) {
        return;
    }
    typeRegistry_->registerType(entry.typeId, renderData);
}

RenderTypeData LevelInitSystem::buildRenderData(const ArchetypeEntry& entry)
{
    RenderTypeData data{};
    data.layer    = entry.layer;
    data.spriteId = entry.spriteId;
    data.texture  = resolveTexture(entry.spriteId);
    applySpriteDefaults(data, entry.spriteId);
    const AnimationClip* clip = resolveAnimation(entry);
    if (clip != nullptr) {
        applyClipToRenderData(data, clip);
    }
    return data;
}

const sf::Texture* LevelInitSystem::resolveTexture(const std::string& spriteId)
{
    auto textureEntry = manifest_->findTextureById(spriteId);
    if (textureEntry) {
        if (!textures_->has(textureEntry->id)) {
            try {
                textures_->load(textureEntry->id, "client/assets/" + textureEntry->path);
            } catch (...) {
                logMissingAsset(spriteId);
            }
        }
        return textures_->get(textureEntry->id);
    }
    logMissingAsset(spriteId);
    return &textures_->getPlaceholder();
}

const AnimationClip* LevelInitSystem::resolveAnimation(const ArchetypeEntry& entry) const
{
    if (animations_ == nullptr) {
        return nullptr;
    }
    if (!entry.animId.empty()) {
        if (animations_->has(entry.animId)) {
            return animations_->get(entry.animId);
        }
        if (labels_ != nullptr) {
            const auto* mapped = labels_->labelFor(entry.spriteId, entry.animId);
            if (mapped != nullptr && animations_->has(*mapped)) {
                return animations_->get(*mapped);
            }
        }
        logMissingAnimation(entry.spriteId, entry.animId);
        return nullptr;
    }
    return animations_->get(entry.spriteId);
}

void LevelInitSystem::applyClipToRenderData(RenderTypeData& data, const AnimationClip* clip) const
{
    if (clip == nullptr || clip->frames.empty()) {
        return;
    }
    data.animation     = clip;
    data.frameCount    = static_cast<std::uint8_t>(clip->frames.size());
    data.frameDuration = clip->frameTime;
    data.frameWidth    = static_cast<std::uint32_t>(clip->frames.front().width);
    data.frameHeight   = static_cast<std::uint32_t>(clip->frames.front().height);
    data.columns       = 1;
}

void LevelInitSystem::applySpriteDefaults(RenderTypeData& data, const std::string& spriteId) const
{
    if (spriteId != "player_ship" || data.texture == nullptr) {
        return;
    }
    data.frameCount    = 6;
    data.frameDuration = 0.08F;
    auto size          = data.texture->getSize();
    data.columns       = 6;
    data.frameWidth    = data.columns == 0 ? size.x : static_cast<std::uint32_t>(size.x / data.columns);
    data.frameHeight   = 14;
}

void LevelInitSystem::logMissingAsset(const std::string& id) const
{
    std::cerr << "[LevelInitSystem] Missing asset: " << id << '\n';
}

void LevelInitSystem::logMissingAnimation(const std::string& spriteId, const std::string& animId) const
{
    std::cerr << "[LevelInitSystem] Missing animation '" << animId << "' for sprite '" << spriteId << "'\n";
}

void LevelInitSystem::applyBackground(Registry& registry, const LevelInitData& data)
{
    auto entry = manifest_->findTextureById(data.backgroundId);
    if (!entry) {
        return;
    }
    if (!textures_->has(entry->id)) {
        try {
            textures_->load(entry->id, "client/assets/" + entry->path);
        } catch (...) {
            logMissingAsset(entry->id);
            return;
        }
    }
    auto* tex = textures_->get(entry->id);
    if (tex == nullptr) {
        return;
    }
    for (auto id : registry.view<BackgroundScrollComponent>()) {
        registry.destroyEntity(id);
    }
    EntityId e = registry.createEntity();
    registry.emplace<SpriteComponent>(e, SpriteComponent(*tex));
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<BackgroundScrollComponent>(e, BackgroundScrollComponent::create(-50.0F, 0.0F, 0.0F));
    registry.emplace<LayerComponent>(e, LayerComponent::create(RenderLayer::Background));
}
