#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "input/InputBuffer.hpp"
#include "input/InputMapper.hpp"
#include "input/InputSystem.hpp"

#include <gtest/gtest.h>

namespace
{
    struct FakeMapper : public InputMapper
    {
        std::uint16_t nextFlags = 0;
        std::uint16_t pollFlags() const override
        {
            return nextFlags;
        }
    };
} // namespace

TEST(InputSystem, DoesNothingWhenNoFlags)
{
    InputBuffer buffer;
    FakeMapper mapper;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);
    InputCommand out{};
    EXPECT_FALSE(buffer.tryPop(out));
}

TEST(InputSystem, EnqueuesCommandWithSequenceAndPos)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::UpFlag;
    std::uint32_t seq = 5;
    float x = 10.0F, y = 20.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.initialize();
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_EQ(out.flags, InputMapper::UpFlag);
    EXPECT_EQ(out.sequenceId, 5u);
    EXPECT_FLOAT_EQ(out.posX, 10.0F);
    EXPECT_FLOAT_EQ(out.posY, 20.0F);
}

TEST(InputSystem, IncrementsSequenceEachPush)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::RightFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    sys.update(registry, 0.0F);
    InputCommand first{};
    InputCommand second{};
    ASSERT_TRUE(buffer.tryPop(first));
    ASSERT_TRUE(buffer.tryPop(second));
    EXPECT_EQ(first.sequenceId, 0u);
    EXPECT_EQ(second.sequenceId, 1u);
}

TEST(InputSystem, SetsLeftAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::LeftFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_FLOAT_EQ(out.angle, 180.0F);
}

TEST(InputSystem, SetsRightAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::RightFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_FLOAT_EQ(out.angle, 0.0F);
}

TEST(InputSystem, SetsUpAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::UpFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_FLOAT_EQ(out.angle, 270.0F);
}

TEST(InputSystem, SetsDownAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::DownFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_FLOAT_EQ(out.angle, 90.0F);
}

TEST(InputSystem, SetsDiagonalUpLeftAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::UpFlag | InputMapper::LeftFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_FLOAT_EQ(out.angle, 225.0F);
}

TEST(InputSystem, SetsDiagonalDownRightAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::DownFlag | InputMapper::RightFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_FLOAT_EQ(out.angle, 45.0F);
}

TEST(InputSystem, FireOnlyKeepsDefaultAngle)
{
    InputBuffer buffer;
    FakeMapper mapper;
    mapper.nextFlags  = InputMapper::FireFlag;
    std::uint32_t seq = 0;
    float x = 0.0F, y = 0.0F;
    InputSystem sys(buffer, mapper, seq, x, y);
    Registry registry;
    sys.update(registry, 0.0F);
    InputCommand out{};
    ASSERT_TRUE(buffer.tryPop(out));
    EXPECT_EQ(out.flags, InputMapper::FireFlag);
    EXPECT_FLOAT_EQ(out.angle, 0.0F);
}
