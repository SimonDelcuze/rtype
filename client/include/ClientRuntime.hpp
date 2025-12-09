#pragma once

#include "ClientConfig.hpp"
#include "animation/AnimationLabels.hpp"
#include "animation/AnimationManifest.hpp"
#include "animation/AnimationRegistry.hpp"
#include "assets/AssetManifest.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "level/LevelState.hpp"
#include "network/ClientInit.hpp"
#include "scheduler/GameLoop.hpp"

#include <atomic>
#include <optional>
#include <thread>

extern std::atomic<bool> g_running;

Window createMainWindow();
std::optional<IpEndpoint> selectServerEndpoint(Window& window, bool useDefault);
AssetManifest loadManifest();
void configureSystems(GameLoop& gameLoop, NetPipelines& net, EntityTypeRegistry& types, const AssetManifest& manifest,
                      TextureManager& textures, AnimationRegistry& animations, AnimationLabels& labels,
                      LevelState& levelState, InputBuffer& inputBuffer, InputMapper& mapper,
                      std::uint32_t& inputSequence, float& playerPosX, float& playerPosY, Window& window);
bool setupNetwork(NetPipelines& net, InputBuffer& inputBuffer, const IpEndpoint& serverEp,
                  std::atomic<bool>& handshakeDone, std::thread& welcomeThread);
void stopNetwork(NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone);
int runClient(const ClientOptions& options);
