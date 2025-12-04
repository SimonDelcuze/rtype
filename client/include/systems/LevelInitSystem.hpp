#pragma once

#include "assets/AssetManifest.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/TextureManager.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "level/LevelState.hpp"
#include "network/LevelInitData.hpp"
#include "systems/ISystem.hpp"

class LevelInitSystem : public ISystem
{
  public:
    LevelInitSystem(ThreadSafeQueue<LevelInitData>& queue, EntityTypeRegistry& typeRegistry,
                    const AssetManifest& manifest, TextureManager& textures, LevelState& state);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    void processLevelInit(const LevelInitData& data);
    void resolveEntityType(const ArchetypeEntry& entry);
    void logMissingAsset(const std::string& id) const;

    ThreadSafeQueue<LevelInitData>* queue_;
    EntityTypeRegistry* typeRegistry_;
    const AssetManifest* manifest_;
    TextureManager* textures_;
    LevelState* state_;
};
