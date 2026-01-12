#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "input/InputSystem.hpp"
#include "systems/AnimationSystem.hpp"
#include "systems/AudioSystem.hpp"
#include "systems/BackgroundScrollSystem.hpp"
#include "systems/DirectionalAnimationSystem.hpp"
#include "systems/FollowerFacingSystem.hpp"
#include "systems/GameOverSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/IntroCinematicSystem.hpp"
#include "systems/InvincibilitySystem.hpp"
#include "systems/LevelEventSystem.hpp"
#include "systems/LevelInitSystem.hpp"
#include "systems/NetworkDebugOverlay.hpp"
#include "systems/NetworkMessageSystem.hpp"
#include "systems/NotificationSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "systems/ReplicationSystem.hpp"

AssetManifest loadManifest()
{
    try {
        return AssetManifest::fromFile("client/assets/assets.json");
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to load asset manifest: " + std::string(e.what()));
        return AssetManifest{};
    }
}

namespace
{
    void preloadSounds(const AssetManifest& manifest, SoundManager& soundManager)
    {
        for (const auto& entry : manifest.getSounds()) {
            if (entry.type == "music") {
                continue;
            }
            try {
                soundManager.load(entry.id, "client/assets/" + entry.path);
            } catch (const std::exception& e) {
                Logger::instance().warn("[Audio] Failed to load sound " + entry.id + ": " + e.what());
            }
        }
    }
} // namespace

void configureSystems(std::uint32_t localPlayerId, RoomType gameMode, GameLoop& gameLoop, NetPipelines& net,
                      EntityTypeRegistry& types, const AssetManifest& manifest, TextureManager& textures,
                      AnimationRegistry& animations, AnimationLabels& labels, LevelState& levelState,
                      InputBuffer& inputBuffer, InputMapper& mapper, std::uint32_t& inputSequence, float& playerPosX,
                      float& playerPosY, Window& window, FontManager& fontManager, EventBus& eventBus,
                      GraphicsFactory& graphicsFactory, SoundManager& soundManager,
                      ThreadSafeQueue<NotificationData>& broadcastQueue)
{
    preloadSounds(manifest, soundManager);

    gameLoop.addSystem(std::make_shared<IntroCinematicSystem>(levelState));
    gameLoop.addSystem(std::make_shared<InputSystem>(localPlayerId, inputBuffer, mapper, inputSequence, playerPosX,
                                                     playerPosY, textures, animations, &levelState));
    gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
    gameLoop.addSystem(
        std::make_shared<LevelInitSystem>(net.levelInit, types, manifest, textures, animations, labels, levelState));
    gameLoop.addSystem(
        std::make_shared<LevelEventSystem>(net.levelEvents, manifest, textures, g_musicVolume, levelState));
    gameLoop.addSystem(std::make_shared<ReplicationSystem>(net.parsed, net.spawns, net.destroys, types));
    gameLoop.addSystem(std::make_shared<InvincibilitySystem>());
    gameLoop.addSystem(std::make_shared<GameOverSystem>(eventBus, localPlayerId, gameMode));
    gameLoop.addSystem(std::make_shared<FollowerFacingSystem>(animations, labels));
    gameLoop.addSystem(std::make_shared<DirectionalAnimationSystem>(animations, labels));
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<BackgroundScrollSystem>(window));
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));
    gameLoop.addSystem(
        std::make_shared<HUDSystem>(window, fontManager, textures, levelState, localPlayerId, gameMode));
    gameLoop.addSystem(std::make_shared<NetworkDebugOverlay>(window, fontManager));
    gameLoop.addSystem(std::make_shared<AudioSystem>(soundManager, graphicsFactory));
    gameLoop.addSystem(std::make_shared<NotificationSystem>(window, fontManager, broadcastQueue));
}
