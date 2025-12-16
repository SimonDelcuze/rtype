#include "level/EntityTypeSetup.hpp"

namespace
{

    void registerPlayerType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("player_ship");
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto* tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture       = tex;
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

    void registerEnemyType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("mob1");
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto* tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture  = tex;
        data.layer    = 1;
        data.spriteId = "mob1";
        registry.registerType(2, data);
    }

    void registerBulletType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("bullet");
        if (!entry)
            return;
        if (!textures.has(entry->id))
            textures.load(entry->id, "client/assets/" + entry->path);
        auto* tex = textures.get(entry->id);
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

            auto* enemyTex = textures.get(enemyEntry->id);
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
        auto* tex = textures.get(entry->id);
        if (tex == nullptr)
            return;

        RenderTypeData data{};
        data.texture  = tex;
        data.layer    = 1;
        data.spriteId = textureId;
        registry.registerType(typeId, data);
    }

} // namespace

void registerEntityTypes(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
{
    registerPlayerType(registry, textures, manifest);
    registerEnemyType(registry, textures, manifest);
    registerBulletType(registry, textures, manifest);
    registerObstacleType(registry, textures, manifest, 9, "obstacle_top");
    registerObstacleType(registry, textures, manifest, 10, "obstacle_middle");
    registerObstacleType(registry, textures, manifest, 11, "obstacle_bottom");
}
