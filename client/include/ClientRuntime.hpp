#pragma once

#include "ClientConfig.hpp"
#include "animation/AnimationLabels.hpp"
#include "animation/AnimationManifest.hpp"
#include "animation/AnimationRegistry.hpp"
#include "assets/AssetManifest.hpp"
#include "audio/SoundManager.hpp"
#include "events/EventBus.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "input/KeyBindings.hpp"
#include "level/EntityTypeRegistry.hpp"
#include "level/LevelState.hpp"
#include "network/ClientInit.hpp"
#include "network/RoomType.hpp"
#include "network/UdpSocket.hpp"
#include "scheduler/GameLoop.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <thread>

extern std::atomic<bool> g_running;
extern KeyBindings g_keyBindings;
extern float g_musicVolume;
extern bool g_networkDebugEnabled;
extern bool g_isRoomHost;
extern std::uint8_t g_expectedPlayerCount;
extern ColorFilterMode g_colorFilterMode;
extern std::atomic<bool> g_forceExit;

enum class JoinResult
{
    Accepted,
    Denied,
    Timeout
};

struct GameSessionResult
{
    bool retry      = false;
    bool serverLost = false;
    std::optional<int> exitCode;
};

struct ClientLoopResult
{
    bool continueLoop = false;
    std::optional<int> exitCode;
};

Window createMainWindow();
std::optional<IpEndpoint> selectServerEndpoint(Window& window, bool useDefault,
                                               ThreadSafeQueue<NotificationData>& broadcastQueue);
AssetManifest loadManifest();
void configureSystems(std::uint32_t localPlayerId, RoomType gameMode, GameLoop& gameLoop, NetPipelines& net,
                      EntityTypeRegistry& types, const AssetManifest& manifest, TextureManager& textures,
                      AnimationRegistry& animations, AnimationLabels& labels, LevelState& levelState,
                      InputBuffer& inputBuffer, InputMapper& mapper, std::uint32_t& inputSequence, float& playerPosX,
                      float& playerPosY, Window& window, FontManager& fontManager, EventBus& eventBus,
                      GraphicsFactory& graphicsFactory, SoundManager& soundManager,
                      ThreadSafeQueue<NotificationData>& broadcastQueue);
bool setupNetwork(NetPipelines& net, InputBuffer& inputBuffer, const IpEndpoint& serverEp,
                  std::atomic<bool>& handshakeDone, std::thread& welcomeThread,
                  ThreadSafeQueue<NotificationData>* broadcastQueue = nullptr);
void stopNetwork(NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone);
JoinResult waitForJoinResponse(Window& window, NetPipelines& net, float timeoutSeconds = 5.0F);
bool runWaitingRoom(Window& window, NetPipelines& net, const IpEndpoint& serverEp, std::string& errorMessage,
                    ThreadSafeQueue<NotificationData>& broadcastQueue, bool& serverLost);
void showErrorMessage(Window& window, const std::string& message, float displayTime = 3.0F);
std::optional<IpEndpoint> showConnectionMenu(Window& window, FontManager& fontManager, TextureManager& textureManager,
                                             std::string& errorMessage,
                                             ThreadSafeQueue<NotificationData>& broadcastQueue);
std::optional<IpEndpoint> showLobbyMenuAndGetGameEndpoint(Window& window, const IpEndpoint& lobbyEndpoint,
                                                          RoomType targetRoomType, FontManager& fontManager,
                                                          TextureManager& textureManager,
                                                          ThreadSafeQueue<NotificationData>& broadcastQueue,
                                                          class LobbyConnection* authenticatedConnection,
                                                          bool& serverLost);
std::optional<std::pair<IpEndpoint, RoomType>>
resolveServerEndpoint(const ClientOptions& options, Window& window, FontManager& fontManager,
                      TextureManager& textureManager, std::string& errorMessage,
                      ThreadSafeQueue<NotificationData>& broadcastQueue, std::optional<IpEndpoint>& lastLobbyEndpoint,
                      std::uint32_t& outUserId);
std::optional<int> handleJoinFailure(JoinResult joinResult, Window& window, const ClientOptions& options,
                                     NetPipelines& net, std::thread& welcomeThread, std::atomic<bool>& handshakeDone,
                                     std::string& errorMessage, ThreadSafeQueue<NotificationData>& broadcastQueue);
GameSessionResult runGameSession(std::uint32_t localPlayerId, RoomType gameMode, Window& window,
                                 const ClientOptions& options, const IpEndpoint& serverEndpoint, NetPipelines& net,
                                 InputBuffer& inputBuffer, TextureManager& textureManager, FontManager& fontManager,
                                 std::string& errorMessage, ThreadSafeQueue<NotificationData>& broadcastQueue);
ClientLoopResult runClientIteration(const ClientOptions& options, Window& window, FontManager& fontManager,
                                    TextureManager& textureManager, std::string& errorMessage,
                                    ThreadSafeQueue<NotificationData>& broadcastQueue,
                                    std::optional<IpEndpoint>& lastLobbyEndpoint);
int runClient(const ClientOptions& options);
