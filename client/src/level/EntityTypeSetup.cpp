#include "level/EntityTypeSetup.hpp"

#include <iostream>
namespace
{

    void registerPlayerType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("player_ship");
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture = tex;

        data.layer         = 0;
        data.spriteId      = "player_ship";
        auto size          = tex->getSize();
        data.frameCount    = 6;
        data.frameDuration = 0.08F;
        data.columns       = 6;
        data.frameWidth    = data.columns == 0 ? size.x : static_cast<std::uint32_t>(size.x / data.columns);
        data.frameHeight   = 14;
        registry.registerType(1, data);
    }

    void registerWalkingMobType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest,
                                std::uint16_t typeId, const std::string& textureId, float frameHeight)
    {
        auto entry = manifest.findTextureById(textureId);
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture     = tex;
        data.layer       = 1;
        data.spriteId    = textureId;
        data.frameHeight = static_cast<std::uint32_t>(frameHeight);
        registry.registerType(typeId, data);
    }

    void registerBulletType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("bullet");
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture  = tex;
        data.layer    = 1;
        data.spriteId = "bullet";
        registry.registerType(3, data);

        auto enemyEntry = manifest.findTextureById("enemy_bullet");
        if (enemyEntry) {
            if (!textures.has(enemyEntry->id))
                textures.load(enemyEntry->id, "client/assets/" + enemyEntry->path);

            auto enemyTex = textures.get(enemyEntry->id);
            if (enemyTex) {
                RenderTypeData enemyData{};
                enemyData.texture  = enemyTex;
                enemyData.layer    = 1;
                enemyData.spriteId = "enemy_bullet";
                registry.registerType(15, enemyData);
            }
        }
    }

    void registerObstacleType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest,
                              std::uint16_t typeId, const std::string& textureId)
    {
        auto entry = manifest.findTextureById(textureId);
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture  = tex;
        data.layer    = 1;
        data.spriteId = textureId;
        registry.registerType(typeId, data);
    }

    void registerAllyType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("player_ship");
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture       = tex;
        data.layer         = 1;
        data.spriteId      = "player_ship";
        data.frameCount    = 1;
        data.frameDuration = 0.08F;
        data.columns       = 6;
        data.frameWidth    = 33;
        data.frameHeight   = 14;
        data.defaultScaleX = 0.5F;
        data.defaultScaleY = 0.5F;
        registry.registerType(24, data);
    }

    void registerShieldType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("shield");
        if (!entry) {
            std::cerr << "[EntityTypeSetup] Shield texture 'shield' not found in manifest" << std::endl;
            return;
        }
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto tex = textures.get(entry->id);
        if (tex == nullptr) {
            std::cerr << "[EntityTypeSetup] Failed to load shield texture" << std::endl;
            return;
        }

        RenderTypeData data{};
        data.texture       = tex;
        data.layer         = 1;
        data.spriteId      = "shield";
        data.frameCount    = 1;
        data.frameDuration = 0.20F;
        data.columns       = 1;
        data.frameWidth    = 106;
        data.frameHeight   = 125;
        data.defaultScaleX = 0.3F;
        data.defaultScaleY = 0.3F;
        registry.registerType(25, data);
        std::cerr << "[EntityTypeSetup] Registered shield type 25" << std::endl;
    }

} // namespace

void registerEntityTypes(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest,
                         const AnimationRegistry* animations, const AnimationLabels* labels)
{
    (void) animations;
    (void) labels;
    registerPlayerType(registry, textures, manifest);
    registerWalkingMobType(registry, textures, manifest, 2, "mob1", 36.0F);
    registerWalkingMobType(registry, textures, manifest, 21, "mob2", 32.0F);
    registerWalkingMobType(registry, textures, manifest, 23, "follower", 33.0F);
    registerBulletType(registry, textures, manifest);
    registerObstacleType(registry, textures, manifest, 9, "obstacle_top");
    registerObstacleType(registry, textures, manifest, 10, "obstacle_middle");
    registerObstacleType(registry, textures, manifest, 11, "obstacle_bottom");
    registerAllyType(registry, textures, manifest);
    registerShieldType(registry, textures, manifest);
}
