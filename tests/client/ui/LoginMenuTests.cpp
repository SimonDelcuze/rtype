#include "components/InputFieldComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/LoginMenu.hpp"

#include <atomic>
#include <filesystem>
#include <gtest/gtest.h>

class LoginMenuTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Try to find assets in common locations
        std::string assetPath = "client/assets/";
        if (!std::filesystem::exists(assetPath)) {
            assetPath = "../../../client/assets/";
        }

        if (std::filesystem::exists(assetPath)) {
            try {
                fonts.load("ui", assetPath + "fonts/ui.ttf");
                textures.load("menu_bg", assetPath + "backgrounds/menu.jpg");
                textures.load("logo", assetPath + "other/rtype-logo.png");
            } catch (...) {
                // If loading fails, we'll still try to run the tests
            }
        }
    }

    FontManager fonts;
    TextureManager textures;
    std::atomic<bool> running{true};
    LobbyConnection lobbyConn{IpEndpoint::v4(127, 0, 0, 1, 1234), running};
    ThreadSafeQueue<NotificationData> broadcastQueue;
    Registry registry;
};

TEST_F(LoginMenuTest, CreatePopulatesRegistry)
{
    LoginMenu menu(fonts, textures, lobbyConn, broadcastQueue);

    // If assets are not loaded, create() might throw. We catch it to still check registry if possible.
    try {
        menu.create(registry);
    } catch (...) {
    }

    std::size_t inputCount = 0;
    for (auto entity : registry.view<InputFieldComponent>()) {
        (void) entity;
        inputCount++;
    }
    EXPECT_GE(inputCount, 2);

    std::size_t transformCount = 0;
    for (auto entity : registry.view<TransformComponent>()) {
        (void) entity;
        transformCount++;
    }
    EXPECT_GE(transformCount, 4);
}

TEST_F(LoginMenuTest, GetResultInitialState)
{
    LoginMenu menu(fonts, textures, lobbyConn, broadcastQueue);
    auto result = menu.getResult(registry);

    EXPECT_FALSE(result.authenticated);
    EXPECT_FALSE(result.openRegister);
    EXPECT_FALSE(result.backRequested);
    EXPECT_FALSE(result.exitRequested);
}

TEST_F(LoginMenuTest, ResetClearsState)
{
    LoginMenu menu(fonts, textures, lobbyConn, broadcastQueue);
    menu.reset();
    auto result = menu.getResult(registry);
    EXPECT_FALSE(result.authenticated);
}
