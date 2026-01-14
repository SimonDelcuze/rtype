#include "ClientRuntime.hpp"

#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "auth/AuthResult.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "level/EntityTypeSetup.hpp"

std::atomic<bool> g_running{true};
KeyBindings g_keyBindings          = KeyBindings::defaults();
float g_musicVolume                = 20.0F;
bool g_networkDebugEnabled         = false;
bool g_isRoomHost                  = false;
bool g_joinAsSpectator             = false;
std::uint8_t g_expectedPlayerCount = 0;
ColorFilterMode g_colorFilterMode  = ColorFilterMode::None;
std::atomic<bool> g_forceExit{false};

int runClient(const ClientOptions& options)
{
    g_forceExit = false;
    configureLogger(options.verbose);

    Window window = createMainWindow();

    FontManager fontManager;
    TextureManager textureManager;
    std::string errorMessage;
    ThreadSafeQueue<NotificationData> broadcastQueue;
    std::optional<IpEndpoint> lastLobbyEndpoint;
    AuthResult preservedAuth{};

    while (window.isOpen() && g_running) {
        Logger::instance().info("[Client] Starting new iteration...");
        ClientLoopResult result = runClientIteration(options, window, fontManager, textureManager, errorMessage,
                                                     broadcastQueue, lastLobbyEndpoint, &preservedAuth);
        if (result.exitCode.has_value())
            return *result.exitCode;
        if (!result.continueLoop)
            break;
    }

    return 0;
}
