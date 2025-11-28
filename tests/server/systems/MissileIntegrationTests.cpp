#include "systems/MovementSystem.hpp"
#include "systems/PlayerInputSystem.hpp"

#include <gtest/gtest.h>

static ReceivedInput inputWithFlags(std::uint32_t playerId, std::uint16_t seq, std::uint16_t flags, float x, float y,
                                    float angle)
{
    ServerInput si{};
    si.playerId   = playerId;
    si.sequenceId = seq;
    si.flags      = flags;
    si.x          = x;
    si.y          = y;
    si.angle      = angle;
    return ReceivedInput{si, IpEndpoint::v4(0, 0, 0, 0, 0)};
}

TEST(MissileIntegration, MissileFiresAndMovesForward)
{
    Registry registry;
    EntityId player = registry.createEntity();
    auto& t         = registry.emplace<TransformComponent>(player);
    t.x             = 0.0F;
    t.y             = 0.0F;
    auto& pic       = registry.emplace<PlayerInputComponent>(player);
    pic.angle       = 0.0F;
    registry.emplace<VelocityComponent>(player);

    PlayerInputSystem inputSys(1.0F, 10.0F, 2.0F, 2);
    std::vector<ReceivedInput> inputs;
    inputs.push_back(inputWithFlags(player, 1, static_cast<std::uint16_t>(InputFlag::Fire), t.x, t.y, pic.angle));
    inputSys.update(registry, inputs);

    MovementSystem move;
    move.update(registry, 0.5F);

    std::size_t missiles = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        if (!registry.has<MissileComponent>(id))
            continue;
        ++missiles;
        auto& mt = registry.get<TransformComponent>(id);
        auto& mv = registry.get<VelocityComponent>(id);
        auto& mc = registry.get<MissileComponent>(id);
        EXPECT_FLOAT_EQ(mt.x, mv.vx * 0.5F);
        EXPECT_FLOAT_EQ(mt.y, mv.vy * 0.5F);
        EXPECT_FLOAT_EQ(mc.lifetime, 2.0F);
        EXPECT_TRUE(mc.fromPlayer);
    }
    EXPECT_EQ(missiles, 1u);
}

TEST(MissileIntegration, MissileRespectsAngle)
{
    Registry registry;
    EntityId player = registry.createEntity();
    auto& t         = registry.emplace<TransformComponent>(player);
    t.x             = -2.0F;
    t.y             = 3.0F;
    auto& pic       = registry.emplace<PlayerInputComponent>(player);
    pic.angle       = static_cast<float>(M_PI) / 4.0F;
    registry.emplace<VelocityComponent>(player);

    PlayerInputSystem inputSys(1.0F, 8.0F, 1.5F, 1);
    std::vector<ReceivedInput> inputs;
    inputs.push_back(inputWithFlags(player, 1, static_cast<std::uint16_t>(InputFlag::Fire), t.x, t.y, pic.angle));
    inputSys.update(registry, inputs);

    EntityId missileId = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (registry.isAlive(id) && registry.has<MissileComponent>(id)) {
            missileId = id;
            break;
        }
    }
    ASSERT_NE(missileId, 0u);
    auto start = registry.get<TransformComponent>(missileId);
    auto vel   = registry.get<VelocityComponent>(missileId);

    MovementSystem move;
    move.update(registry, 0.25F);

    auto& mt = registry.get<TransformComponent>(missileId);
    EXPECT_NEAR(vel.vx, std::cos(pic.angle) * 8.0F, 1e-5);
    EXPECT_NEAR(vel.vy, std::sin(pic.angle) * 8.0F, 1e-5);
    EXPECT_NEAR(mt.x, start.x + vel.vx * 0.25F, 1e-5);
    EXPECT_NEAR(mt.y, start.y + vel.vy * 0.25F, 1e-5);
}

TEST(MissileIntegration, MultipleMissilesFromSequentialInputs)
{
    Registry registry;
    EntityId player = registry.createEntity();
    auto& t         = registry.emplace<TransformComponent>(player);
    t.x             = 5.0F;
    t.y             = -1.0F;
    auto& pic       = registry.emplace<PlayerInputComponent>(player);
    registry.emplace<VelocityComponent>(player);

    PlayerInputSystem inputSys(1.0F, 5.0F, 3.0F, 1);
    std::vector<ReceivedInput> inputs;
    inputs.push_back(inputWithFlags(player, 1, static_cast<std::uint16_t>(InputFlag::Fire), t.x, t.y, 0.0F));
    inputs.push_back(inputWithFlags(player, 2, static_cast<std::uint16_t>(InputFlag::Fire), t.x, t.y, 0.0F));
    inputSys.update(registry, inputs);

    EXPECT_GE(registry.entityCount(), 3u);
    std::size_t missiles = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id) || !registry.has<MissileComponent>(id))
            continue;
        ++missiles;
    }
    EXPECT_EQ(missiles, 2u);
}
