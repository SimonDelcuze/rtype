#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "ui/MenuRunner.hpp"

#include <atomic>
#include <gtest/gtest.h>

class MenuRunnerTest : public ::testing::Test
{
  protected:
    Window window{Vector2u{100, 100}, "Test"};
    FontManager fonts;
    TextureManager textures;
    std::atomic<bool> running{true};
    ThreadSafeQueue<NotificationData> broadcastQueue;
};

TEST_F(MenuRunnerTest, GetRegistry)
{
    MenuRunner runner(window, fonts, textures, running, broadcastQueue);
    auto& registry = runner.getRegistry();
    (void) registry;
}
