#include "components/InputHistoryComponent.hpp"

#include <gtest/gtest.h>

TEST(InputHistoryComponent, DefaultState)
{
    InputHistoryComponent history;

    EXPECT_EQ(history.size(), 0u);
    EXPECT_EQ(history.lastAcknowledgedSequence, 0u);
}

TEST(InputHistoryComponent, PushInput)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);

    EXPECT_EQ(history.size(), 1u);
    EXPECT_EQ(history.history.front().sequenceId, 1u);
    EXPECT_EQ(history.history.front().flags, 0x01);
    EXPECT_FLOAT_EQ(history.history.front().posX, 10.0F);
    EXPECT_FLOAT_EQ(history.history.front().posY, 20.0F);
    EXPECT_FLOAT_EQ(history.history.front().angle, 0.5F);
}

TEST(InputHistoryComponent, PushMultipleInputs)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);
    history.pushInput(3, 0x03, 20.0F, 30.0F, 0.7F);

    EXPECT_EQ(history.size(), 3u);
    EXPECT_EQ(history.history[0].sequenceId, 1u);
    EXPECT_EQ(history.history[1].sequenceId, 2u);
    EXPECT_EQ(history.history[2].sequenceId, 3u);
}

TEST(InputHistoryComponent, AcknowledgeUpTo)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);
    history.pushInput(3, 0x03, 20.0F, 30.0F, 0.7F);

    history.acknowledgeUpTo(2);

    EXPECT_EQ(history.size(), 1u);
    EXPECT_EQ(history.lastAcknowledgedSequence, 2u);
    EXPECT_EQ(history.history.front().sequenceId, 3u);
}

TEST(InputHistoryComponent, AcknowledgeAll)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);
    history.pushInput(3, 0x03, 20.0F, 30.0F, 0.7F);

    history.acknowledgeUpTo(3);

    EXPECT_EQ(history.size(), 0u);
    EXPECT_EQ(history.lastAcknowledgedSequence, 3u);
}

TEST(InputHistoryComponent, AcknowledgeNone)
{
    InputHistoryComponent history;

    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);
    history.pushInput(3, 0x03, 20.0F, 30.0F, 0.7F);

    history.acknowledgeUpTo(1);

    EXPECT_EQ(history.size(), 2u);
    EXPECT_EQ(history.lastAcknowledgedSequence, 1u);
}

TEST(InputHistoryComponent, GetInputsAfter)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);
    history.pushInput(3, 0x03, 20.0F, 30.0F, 0.7F);
    history.pushInput(4, 0x04, 25.0F, 35.0F, 0.8F);

    auto inputs = history.getInputsAfter(2);

    EXPECT_EQ(inputs.size(), 2u);
    EXPECT_EQ(inputs[0].sequenceId, 3u);
    EXPECT_EQ(inputs[1].sequenceId, 4u);
}

TEST(InputHistoryComponent, GetInputsAfterNone)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);

    auto inputs = history.getInputsAfter(2);

    EXPECT_EQ(inputs.size(), 0u);
}

TEST(InputHistoryComponent, GetInputsAfterAll)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);

    auto inputs = history.getInputsAfter(0);

    EXPECT_EQ(inputs.size(), 2u);
}

TEST(InputHistoryComponent, Clear)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F);
    history.pushInput(2, 0x02, 15.0F, 25.0F, 0.6F);
    history.acknowledgeUpTo(1);

    history.clear();

    EXPECT_EQ(history.size(), 0u);
    EXPECT_EQ(history.lastAcknowledgedSequence, 0u);
}

TEST(InputHistoryComponent, MaxHistorySizeLimit)
{
    InputHistoryComponent history;
    history.maxHistorySize = 10;

    for (std::uint32_t i = 0; i < 15; ++i) {
        history.pushInput(i, 0, 0.0F, 0.0F, 0.0F);
    }

    EXPECT_EQ(history.size(), 10u);
    EXPECT_EQ(history.history.front().sequenceId, 5u);
    EXPECT_EQ(history.history.back().sequenceId, 14u);
}

TEST(InputHistoryComponent, DeltaTimeStored)
{
    InputHistoryComponent history;

    history.pushInput(1, 0x01, 10.0F, 20.0F, 0.5F, 0.033F);

    EXPECT_FLOAT_EQ(history.history.front().deltaTime, 0.033F);
}
