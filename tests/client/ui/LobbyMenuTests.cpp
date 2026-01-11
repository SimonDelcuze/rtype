#include "components/ButtonComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/LobbyMenu.hpp"

#include <atomic>
#include <gtest/gtest.h>

class LobbyMenuTest : public ::testing::Test
{
  protected:
    FontManager fonts;
    TextureManager textures;
    std::atomic<bool> running{true};
    IpEndpoint lobbyEndpoint = IpEndpoint::v4(127, 0, 0, 1, 1234);
    ThreadSafeQueue<NotificationData> broadcastQueue;
    Registry registry;
};

TEST_F(LobbyMenuTest, CreatePopulatesRegistry)
{
    LobbyMenu menu(fonts, textures, lobbyEndpoint, broadcastQueue, running);

    menu.create(registry);

    std::size_t buttonCount = 0;
    for (auto entity : registry.view<ButtonComponent>()) {
        (void) entity;
        buttonCount++;
    }
    EXPECT_GE(buttonCount, 3);

    std::size_t transformCount = 0;
    for (auto entity : registry.view<TransformComponent>()) {
        (void) entity;
        transformCount++;
    }
    EXPECT_GE(transformCount, 5);
}

TEST_F(LobbyMenuTest, InitialState)
{
    LobbyMenu menu(fonts, textures, lobbyEndpoint, broadcastQueue, running);
    auto result = menu.getResult(registry);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.exitRequested);
    EXPECT_FALSE(result.backRequested);
}
