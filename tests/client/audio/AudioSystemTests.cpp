#include "audio/SoundManager.hpp"
#include "components/AudioComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "systems/AudioSystem.hpp"

#include <gtest/gtest.h>

class AudioSystemTests : public ::testing::Test
{
  protected:
    Registry registry;
    GraphicsFactory graphicsFactory;
    SoundManager soundManager;
    AudioSystem audioSystem{soundManager, graphicsFactory};

    AudioSystemTests() {}
};

TEST_F(AudioSystemTests, UpdateWithNoEntities)
{
    EXPECT_NO_THROW(audioSystem.update(registry, 0.0f));
}

TEST_F(AudioSystemTests, UpdateWithEntityNoAudioComponent)
{
    EntityId entity = registry.createEntity();
    (void) entity;

    EXPECT_NO_THROW(audioSystem.update(registry, 0.0f));
}

TEST_F(AudioSystemTests, UpdateWithAudioComponentNoAction)
{
    EntityId entity = registry.createEntity();
    registry.emplace<AudioComponent>(entity);

    EXPECT_NO_THROW(audioSystem.update(registry, 0.0f));
}

TEST_F(AudioSystemTests, PlayActionWithMissingSound)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.play("nonexistent");

    audioSystem.update(registry, 0.0f);

    EXPECT_EQ(audio.action, AudioAction::None);
    EXPECT_FALSE(audio.isPlaying);
}

TEST_F(AudioSystemTests, StopActionResetsIsPlaying)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.isPlaying = true;
    audio.stop();

    audioSystem.update(registry, 0.0f);

    EXPECT_EQ(audio.action, AudioAction::None);
    EXPECT_FALSE(audio.isPlaying);
}

TEST_F(AudioSystemTests, PauseActionResetsIsPlaying)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.isPlaying = true;
    audio.pause();

    audioSystem.update(registry, 0.0f);

    EXPECT_EQ(audio.action, AudioAction::None);
    EXPECT_FALSE(audio.isPlaying);
}

TEST_F(AudioSystemTests, DeadEntityIsSkipped)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.play("test");

    registry.destroyEntity(entity);

    EXPECT_NO_THROW(audioSystem.update(registry, 0.0f));
}

TEST_F(AudioSystemTests, MultipleEntitiesProcessed)
{
    EntityId entity1 = registry.createEntity();
    EntityId entity2 = registry.createEntity();

    registry.emplace<AudioComponent>(entity1);
    registry.emplace<AudioComponent>(entity2);

    auto& audio1 = registry.get<AudioComponent>(entity1);
    auto& audio2 = registry.get<AudioComponent>(entity2);

    audio1.isPlaying = true;
    audio1.stop();
    audio2.isPlaying = true;
    audio2.stop();

    audioSystem.update(registry, 0.0f);

    auto& result1 = registry.get<AudioComponent>(entity1);
    auto& result2 = registry.get<AudioComponent>(entity2);
    EXPECT_EQ(result1.action, AudioAction::None);
    EXPECT_EQ(result2.action, AudioAction::None);
    EXPECT_FALSE(result1.isPlaying);
    EXPECT_FALSE(result2.isPlaying);
}

TEST_F(AudioSystemTests, VolumeAndPitchAreRespected)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.volume    = 50.0F;
    audio.pitch     = 1.5F;
    audio.play("test");

    audioSystem.update(registry, 0.0f);

    EXPECT_FLOAT_EQ(audio.volume, 50.0F);
    EXPECT_FLOAT_EQ(audio.pitch, 1.5F);
}

TEST_F(AudioSystemTests, LoopSettingPreserved)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.loop      = true;
    audio.play("test");

    audioSystem.update(registry, 0.0f);

    EXPECT_TRUE(audio.loop);
}
