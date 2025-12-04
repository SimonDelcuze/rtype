#include "systems/LevelInitSystem.hpp"

#include <iostream>

LevelInitSystem::LevelInitSystem(ThreadSafeQueue<LevelInitData>& queue, EntityTypeRegistry& typeRegistry,
                                 const AssetManifest& manifest, TextureManager& textures, LevelState& state)
    : queue_(&queue), typeRegistry_(&typeRegistry), manifest_(&manifest), textures_(&textures), state_(&state)
{}

void LevelInitSystem::initialize() {}

void LevelInitSystem::update(Registry&, float)
{
    LevelInitData data;
    while (queue_->tryPop(data)) {
        processLevelInit(data);
    }
}

void LevelInitSystem::cleanup() {}

void LevelInitSystem::processLevelInit(const LevelInitData& data)
{
    typeRegistry_->clear();
    state_->levelId = data.levelId;
    state_->seed    = data.seed;
    state_->active  = true;

    for (const auto& entry : data.archetypes) {
        resolveEntityType(entry);
    }
}

void LevelInitSystem::resolveEntityType(const ArchetypeEntry& entry)
{
    RenderTypeData renderData{};
    renderData.layer = entry.layer;

    auto textureEntry = manifest_->findTextureById(entry.spriteId);
    if (textureEntry) {
        if (!textures_->has(textureEntry->id)) {
            try {
                textures_->load(textureEntry->id, textureEntry->path);
            } catch (...) {
                logMissingAsset(entry.spriteId);
            }
        }
        renderData.texture = textures_->get(textureEntry->id);
    }

    if (renderData.texture == nullptr) {
        logMissingAsset(entry.spriteId);
        renderData.texture = &textures_->getPlaceholder();
    }

    typeRegistry_->registerType(entry.typeId, renderData);
}

void LevelInitSystem::logMissingAsset(const std::string& id) const
{
    std::cerr << "[LevelInitSystem] Missing asset: " << id << '\n';
}
