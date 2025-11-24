#include "audio/SoundManager.hpp"
#include "components/AudioComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/AudioSystem.hpp"

#include <gtest/gtest.h>

class AudioSystemTests : public ::testing::Test
{
  protected:
    Registry registry;
    SoundManager soundManager;
};

TEST_F(AudioSystemTests, UpdateWithNoEntities)
{
    AudioSystem system(soundManager);
    EXPECT_NO_THROW(system.update(registry));
}

TEST_F(AudioSystemTests, UpdateWithEntityNoAudioComponent)
{
    EntityId entity = registry.createEntity();
    (void) entity;

    AudioSystem system(soundManager);
    EXPECT_NO_THROW(system.update(registry));
}

TEST_F(AudioSystemTests, UpdateWithAudioComponentNoAction)
{
    EntityId entity = registry.createEntity();
    registry.emplace<AudioComponent>(entity);

    AudioSystem system(soundManager);
    EXPECT_NO_THROW(system.update(registry));
}

TEST_F(AudioSystemTests, PlayActionWithMissingSound)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.play("nonexistent");

    AudioSystem system(soundManager);
    system.update(registry);

    // Action should be reset to None after processing
    EXPECT_EQ(audio.action, AudioAction::None);
    // isPlaying should remain false since sound wasn't found
    EXPECT_FALSE(audio.isPlaying);
}

TEST_F(AudioSystemTests, StopActionResetsIsPlaying)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.isPlaying = true;
    audio.stop();

    AudioSystem system(soundManager);
    system.update(registry);

    EXPECT_EQ(audio.action, AudioAction::None);
    EXPECT_FALSE(audio.isPlaying);
}

TEST_F(AudioSystemTests, PauseActionResetsIsPlaying)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.isPlaying = true;
    audio.pause();

    AudioSystem system(soundManager);
    system.update(registry);

    EXPECT_EQ(audio.action, AudioAction::None);
    EXPECT_FALSE(audio.isPlaying);
}

TEST_F(AudioSystemTests, DeadEntityIsSkipped)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.play("test");

    registry.destroyEntity(entity);

    AudioSystem system(soundManager);
    EXPECT_NO_THROW(system.update(registry));
}

TEST_F(AudioSystemTests, MultipleEntitiesProcessed)
{
    EntityId entity1 = registry.createEntity();
    EntityId entity2 = registry.createEntity();

    auto& audio1 = registry.emplace<AudioComponent>(entity1);
    auto& audio2 = registry.emplace<AudioComponent>(entity2);

    audio1.play("sound1");
    audio2.play("sound2");

    AudioSystem system(soundManager);
    system.update(registry);

    // Both entities should have their actions reset after processing
    auto& updatedAudio1 = registry.get<AudioComponent>(entity1);
    auto& updatedAudio2 = registry.get<AudioComponent>(entity2);
    EXPECT_EQ(updatedAudio1.action, AudioAction::None);
    EXPECT_EQ(updatedAudio2.action, AudioAction::None);
}

TEST_F(AudioSystemTests, VolumeAndPitchAreRespected)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.volume    = 50.0F;
    audio.pitch     = 1.5F;
    audio.play("test");

    AudioSystem system(soundManager);
    system.update(registry);

    // Values should be preserved
    EXPECT_FLOAT_EQ(audio.volume, 50.0F);
    EXPECT_FLOAT_EQ(audio.pitch, 1.5F);
}

TEST_F(AudioSystemTests, LoopSettingPreserved)
{
    EntityId entity = registry.createEntity();
    auto& audio     = registry.emplace<AudioComponent>(entity);
    audio.loop      = true;
    audio.play("test");

    AudioSystem system(soundManager);
    system.update(registry);

    EXPECT_TRUE(audio.loop);
}
