#pragma once

#include "ClientConfig.hpp"
#include "animation/AnimationLabels.hpp"
#include "animation/AnimationManifest.hpp"
#include "animation/AnimationRegistry.hpp"
#include "assets/AssetManifest.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "level/LevelState.hpp"
#include "network/ClientInit.hpp"
#include "scheduler/GameLoop.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <thread>

extern std::atomic<bool> g_running;

enum class JoinResult
{
    Accepted,
    Denied,
    Timeout
};

struct ClientLoopResult
{
    bool continueLoop = false;
    std::optional<int> exitCode;
};

Window createMainWindow();
std::optional<IpEndpoint> selectServerEndpoint(Window& window, bool useDefault);
AssetManifest loadManifest();
void configureSystems(GameLoop& gameLoop, NetPipelines& net, EntityTypeRegistry& types, const AssetManifest& manifest,
                      TextureManager& textures, AnimationRegistry& animations, AnimationLabels& labels,
                      LevelState& levelState, InputBuffer& inputBuffer, InputMapper& mapper,
                      std::uint32_t& inputSequence, float& playerPosX, float& playerPosY, Window& window,
                      FontManager& fontManager);
bool setupNetwork(NetPipelines& net, InputBuffer& inputBuffer, const IpEndpoint& serverEp,
                  std::atomic<bool>& handshakeDone, std::thread& welcomeThread);
void stopNetwork(NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone);
JoinResult waitForJoinResponse(Window& window, NetPipelines& net, float timeoutSeconds = 5.0F);
bool runWaitingRoom(Window& window, NetPipelines& net, const IpEndpoint& serverEp);
void showErrorMessage(Window& window, const std::string& message, float displayTime = 3.0F);
std::optional<IpEndpoint> showConnectionMenu(Window& window, FontManager& fontManager, TextureManager& textureManager,
                                             std::string& errorMessage);
std::optional<IpEndpoint> resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                                                TextureManager& textureManager, std::string& errorMessage);
std::optional<int> handleJoinFailure(JoinResult joinResult, Window& window, const ClientOptions& options,
                                     NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone,
                                     std::string& errorMessage);
std::optional<int> runGameSession(Window& window, const ClientOptions& options, const IpEndpoint& serverEndpoint,
                                  NetPipelines& net, InputBuffer& inputBuffer, TextureManager& textureManager,
                                  FontManager& fontManager);
ClientLoopResult runClientIteration(const ClientOptions& options, Window& window, FontManager& fontManager,
                                    TextureManager& textureManager, std::string& errorMessage);
int runClient(const ClientOptions& options);
