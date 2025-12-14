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
    }

    void registerObstacleType(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
    {
        auto entry = manifest.findTextureById("enemy_ship");
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
        data.spriteId = "enemy_ship";
        registry.registerType(9, data);
    }

} // namespace

void registerEntityTypes(EntityTypeRegistry& registry, TextureManager& textures, const AssetManifest& manifest)
{
    registerPlayerType(registry, textures, manifest);
    registerEnemyType(registry, textures, manifest);
    registerBulletType(registry, textures, manifest);
    registerObstacleType(registry, textures, manifest);
}
