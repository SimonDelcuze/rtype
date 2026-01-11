#include "components/InputFieldComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/RegisterMenu.hpp"

#include <atomic>
#include <gtest/gtest.h>

class RegisterMenuTest : public ::testing::Test
{
  protected:
    FontManager fonts;
    TextureManager textures;
    std::atomic<bool> running{true};
    LobbyConnection lobbyConn{IpEndpoint::v4(127, 0, 0, 1, 1234), running};
    ThreadSafeQueue<NotificationData> broadcastQueue;
    Registry registry;
};

TEST_F(RegisterMenuTest, CreatePopulatesRegistry)
{
    RegisterMenu menu(fonts, textures, lobbyConn, broadcastQueue);

    menu.create(registry);

    std::size_t inputCount = 0;
    for (auto entity : registry.view<InputFieldComponent>()) {
        (void) entity;
        inputCount++;
    }
    EXPECT_GE(inputCount, 3);
}

TEST_F(RegisterMenuTest, InitialState)
{
    RegisterMenu menu(fonts, textures, lobbyConn, broadcastQueue);
    auto result = menu.getResult(registry);

    EXPECT_FALSE(result.registered);
    EXPECT_FALSE(result.backToLogin);
}
