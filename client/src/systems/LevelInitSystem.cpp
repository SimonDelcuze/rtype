#include "systems/LevelInitSystem.hpp"

#include "Logger.hpp"
#include "components/BackgroundScrollComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"

#include <iostream>
#include <sstream>

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
    bossMeta_.clear();
    for (const auto& boss : data.bosses) {
        BossMeta meta{};
        meta.name   = boss.name;
        meta.scaleX = boss.scaleX;
        meta.scaleY = boss.scaleY;
        bossMeta_.emplace(boss.typeId, std::move(meta));
    }
    state_->levelId              = data.levelId;
    state_->seed                 = data.seed;
    state_->active               = true;
    state_->introCinematicActive = true;
    state_->introCinematicTime   = 0.0F;
    state_->safeZoneActive       = false;
    applyBackground(registry, data);
    createHUDEntities(registry);
    for (const auto& entry : data.archetypes) {
        resolveEntityType(entry);
    }
}

void LevelInitSystem::createHUDEntities(Registry& registry)
{
    EntityId score = registry.createEntity();
    registry.emplace<TransformComponent>(score, TransformComponent::create(0.0F, 0.0F));
    auto& scoreText   = registry.emplace<TextComponent>(score, TextComponent::create("score_font", 20, Color::White));
    scoreText.content = "SCORE 0000000";
    registry.emplace<ScoreComponent>(score, ScoreComponent::create(0));
    registry.emplace<LayerComponent>(score, LayerComponent::create(100));

    EntityId lives = registry.createEntity();
    registry.emplace<TransformComponent>(lives, TransformComponent::create(10.0F, 680.0F));
    registry.emplace<LivesComponent>(lives, LivesComponent::create(3, 3));
    registry.emplace<LayerComponent>(lives, LayerComponent::create(100));
}

void LevelInitSystem::resolveEntityType(const ArchetypeEntry& entry)
{
    RenderTypeData renderData = buildRenderData(entry);
    auto bossIt               = bossMeta_.find(entry.typeId);
    if (bossIt != bossMeta_.end()) {
        renderData.isBoss        = true;
        renderData.bossName      = bossIt->second.name;
        renderData.defaultScaleX = bossIt->second.scaleX;
        renderData.defaultScaleY = bossIt->second.scaleY;
    }
    if (renderData.texture == nullptr ||
        (entry.animId.size() && renderData.animation == nullptr && animations_ != nullptr && labels_ != nullptr)) {
        return;
    }

    std::ostringstream ss;
    ss << "[LevelInit] Register typeId=" << entry.typeId << " spriteId=" << entry.spriteId;
    if (!entry.animId.empty()) {
        ss << " animId=" << entry.animId;
    }
    ss << " texture=" << (renderData.texture ? "ok" : "missing")
       << " frames=" << static_cast<int>(renderData.frameCount) << " layer=" << static_cast<int>(renderData.layer);
    Logger::instance().info(ss.str());

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

std::shared_ptr<ITexture> LevelInitSystem::resolveTexture(const std::string& spriteId)
{
    auto textureEntry = manifest_->findTextureById(spriteId);
    if (!textureEntry) {
        Logger::instance().warn("[LevelInit] Unknown sprite id=" + spriteId);
        return nullptr;
    }

    if (!textures_->has(textureEntry->id)) {
        try {
            textures_->load(textureEntry->id, "client/assets/" + textureEntry->path);
        } catch (...) {
            Logger::instance().warn("[LevelInit] Failed to load texture id=" + textureEntry->id);
            return nullptr;
        }
    }
    return textures_->get(textureEntry->id);
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
    Logger::instance().info("[LevelInit] Background id=" + data.backgroundId + " path=" + entry->path);
    if (!textures_->has(entry->id)) {
        try {
            textures_->load(entry->id, "client/assets/" + entry->path);
        } catch (...) {
            logMissingAsset(entry->id);
            return;
        }
    }
    auto tex = textures_->get(entry->id);
    if (tex == nullptr) {
        return;
    }
    for (auto id : registry.view<BackgroundScrollComponent>()) {
        registry.destroyEntity(id);
    }
    EntityId e = registry.createEntity();
    registry.emplace<SpriteComponent>(e, SpriteComponent(tex));
    registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
    registry.emplace<BackgroundScrollComponent>(e, BackgroundScrollComponent::create(-50.0F, 0.0F, 0.0F));
    registry.emplace<LayerComponent>(e, LayerComponent::create(RenderLayer::Background));
}
