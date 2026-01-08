#include "ClientRuntime.hpp"

#include "Logger.hpp"
#include "animation/AnimationManifest.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "level/EntityTypeSetup.hpp"

std::atomic<bool> g_running{true};
KeyBindings g_keyBindings = KeyBindings::defaults();
float g_musicVolume       = 20.0F;
bool g_networkDebugEnabled = false;

int runClient(const ClientOptions& options)
{
    configureLogger(options.verbose);

    Window window = createMainWindow();

    FontManager fontManager;
    TextureManager textureManager;
    std::string errorMessage;
    while (window.isOpen() && g_running) {
        ClientLoopResult result = runClientIteration(options, window, fontManager, textureManager, errorMessage);
        if (result.exitCode.has_value())
            return *result.exitCode;
        if (!result.continueLoop)
            break;
    }

    return 0;
}
