#include "assets/AssetManifest.hpp"
#include "components/BackgroundScrollComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/TextureManager.hpp"
#include "level/LevelState.hpp"
#include "systems/LevelEventSystem.hpp"

#include <gtest/gtest.h>

TEST(LevelEventSystemTest, UpdateScrollSpeed)
{
    ThreadSafeQueue<LevelEventData> queue;
    AssetManifest manifest;
    TextureManager textures;
    float musicVolume = 50.0f;
    LevelState state;
    Registry registry;

    LevelEventSystem system(queue, manifest, textures, musicVolume, state);

    EntityId bg = registry.createEntity();
    auto& scroll =
        registry.emplace<BackgroundScrollComponent>(bg, BackgroundScrollComponent::create(-10.0f, 0.0f, 0.0f));

    LevelEventData event;
    event.type = LevelEventType::SetScroll;
    LevelScrollSettings settings;
    settings.mode   = LevelScrollMode::Constant;
    settings.speedX = -100.0f;
    event.scroll    = settings;
    queue.push(event);

    system.update(registry, 0.16f);
    EXPECT_EQ(scroll.speedX, -100.0f);
}
