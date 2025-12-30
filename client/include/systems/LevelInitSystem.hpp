#pragma once

#include "animation/AnimationLabels.hpp"
#include "animation/AnimationRegistry.hpp"
#include "assets/AssetManifest.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/TextureManager.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "level/LevelState.hpp"
#include "network/LevelInitData.hpp"
#include "systems/ISystem.hpp"

#include <unordered_map>

class LevelInitSystem : public ISystem
{
  public:
    LevelInitSystem(ThreadSafeQueue<LevelInitData>& queue, EntityTypeRegistry& typeRegistry,
                    const AssetManifest& manifest, TextureManager& textures, AnimationRegistry& animations,
                    AnimationLabels& labels, LevelState& state);

    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;

  private:
    void processLevelInit(Registry& registry, const LevelInitData& data);
    void resolveEntityType(const ArchetypeEntry& entry);
    RenderTypeData buildRenderData(const ArchetypeEntry& entry);
    std::shared_ptr<ITexture> resolveTexture(const std::string& spriteId);
    const AnimationClip* resolveAnimation(const ArchetypeEntry& entry) const;
    void applyClipToRenderData(RenderTypeData& data, const AnimationClip* clip) const;
    void applySpriteDefaults(RenderTypeData& data, const std::string& spriteId) const;
    void logMissingAsset(const std::string& id) const;
    void logMissingAnimation(const std::string& spriteId, const std::string& animId) const;
    void applyBackground(Registry& registry, const LevelInitData& data);
    void createHUDEntities(Registry& registry);

    ThreadSafeQueue<LevelInitData>* queue_;
    EntityTypeRegistry* typeRegistry_;
    const AssetManifest* manifest_;
    TextureManager* textures_;
    AnimationRegistry* animations_;
    AnimationLabels* labels_;
    LevelState* state_;
    struct BossMeta
    {
        std::string name;
        float scaleX = 1.0F;
        float scaleY = 1.0F;
    };
    std::unordered_map<std::uint16_t, BossMeta> bossMeta_;
};
