#include "components/AudioComponent.hpp"

#include <gtest/gtest.h>

TEST(AudioComponentTests, DefaultConstruction)
{
    AudioComponent audio;

    EXPECT_TRUE(audio.soundId.empty());
    EXPECT_EQ(audio.action, AudioAction::None);
    EXPECT_FLOAT_EQ(audio.volume, 100.0F);
    EXPECT_FLOAT_EQ(audio.pitch, 1.0F);
    EXPECT_FALSE(audio.loop);
    EXPECT_FALSE(audio.isPlaying);
}

TEST(AudioComponentTests, PlaySetsActionAndSoundId)
{
    AudioComponent audio;
    audio.play("explosion");

    EXPECT_EQ(audio.soundId, "explosion");
    EXPECT_EQ(audio.action, AudioAction::Play);
}

TEST(AudioComponentTests, StopSetsAction)
{
    AudioComponent audio;
    audio.play("music");
    audio.stop();

    EXPECT_EQ(audio.action, AudioAction::Stop);
}

TEST(AudioComponentTests, PauseSetsAction)
{
    AudioComponent audio;
    audio.play("music");
    audio.pause();

    EXPECT_EQ(audio.action, AudioAction::Pause);
}

TEST(AudioComponentTests, VolumeAndPitchSettings)
{
    AudioComponent audio;
    audio.volume = 50.0F;
    audio.pitch  = 1.5F;

    EXPECT_FLOAT_EQ(audio.volume, 50.0F);
    EXPECT_FLOAT_EQ(audio.pitch, 1.5F);
}

TEST(AudioComponentTests, LoopSetting)
{
    AudioComponent audio;
    audio.loop = true;

    EXPECT_TRUE(audio.loop);
}

TEST(AudioComponentTests, IsPlayingStatus)
{
    AudioComponent audio;
    EXPECT_FALSE(audio.isPlaying);

    audio.isPlaying = true;
    EXPECT_TRUE(audio.isPlaying);
}

TEST(AudioComponentTests, ChainedPlayCalls)
{
    AudioComponent audio;
    audio.play("sound1");
    EXPECT_EQ(audio.soundId, "sound1");

    audio.play("sound2");
    EXPECT_EQ(audio.soundId, "sound2");
    EXPECT_EQ(audio.action, AudioAction::Play);
}
