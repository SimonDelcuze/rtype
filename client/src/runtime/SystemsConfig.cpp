#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "input/InputSystem.hpp"
#include "systems/AnimationSystem.hpp"
#include "systems/AudioSystem.hpp"
#include "systems/BackgroundScrollSystem.hpp"
#include "systems/DirectionalAnimationSystem.hpp"
#include "systems/GameOverSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/IntroCinematicSystem.hpp"
#include "systems/InvincibilitySystem.hpp"
#include "systems/LevelEventSystem.hpp"
#include "systems/LevelInitSystem.hpp"
#include "systems/NetworkDebugOverlay.hpp"
#include "systems/NetworkMessageSystem.hpp"
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

void configureSystems(GameLoop& gameLoop, NetPipelines& net, EntityTypeRegistry& types, const AssetManifest& manifest,
                      TextureManager& textures, AnimationRegistry& animations, AnimationLabels& labels,
                      LevelState& levelState, InputBuffer& inputBuffer, InputMapper& mapper,
                      std::uint32_t& inputSequence, float& playerPosX, float& playerPosY, Window& window,
                      FontManager& fontManager, EventBus& eventBus, GraphicsFactory& graphicsFactory,
                      SoundManager& soundManager)
{
    gameLoop.addSystem(std::make_shared<IntroCinematicSystem>(levelState));
    gameLoop.addSystem(std::make_shared<InputSystem>(inputBuffer, mapper, inputSequence, playerPosX, playerPosY,
                                                     textures, animations, &levelState));
    gameLoop.addSystem(std::make_shared<NetworkMessageSystem>(*net.handler));
    gameLoop.addSystem(
        std::make_shared<LevelInitSystem>(net.levelInit, types, manifest, textures, animations, labels, levelState));
    gameLoop.addSystem(std::make_shared<LevelEventSystem>(net.levelEvents, manifest, textures, g_musicVolume));
    gameLoop.addSystem(std::make_shared<ReplicationSystem>(net.parsed, net.spawns, net.destroys, types));
    gameLoop.addSystem(std::make_shared<InvincibilitySystem>());
    gameLoop.addSystem(std::make_shared<GameOverSystem>(eventBus));
    gameLoop.addSystem(std::make_shared<DirectionalAnimationSystem>(animations, labels));
    gameLoop.addSystem(std::make_shared<AnimationSystem>());
    gameLoop.addSystem(std::make_shared<BackgroundScrollSystem>(window));
    gameLoop.addSystem(std::make_shared<RenderSystem>(window));
    gameLoop.addSystem(std::make_shared<HUDSystem>(window, fontManager, textures));
    gameLoop.addSystem(std::make_shared<NetworkDebugOverlay>(window, fontManager));
    gameLoop.addSystem(std::make_shared<AudioSystem>(soundManager, graphicsFactory));
}
