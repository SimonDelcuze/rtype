#define _USE_MATH_DEFINES
#include "simulation/PlayerCommand.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/PlayerInputSystem.hpp"

#include <cmath>
#include <gtest/gtest.h>

static PlayerCommand makeInput(std::uint32_t playerId, std::uint16_t seq, std::uint16_t flags, float x = 0.0F,
                               float y = 0.0F, float angle = 0.0F)
{
    PlayerCommand cmd{};
    cmd.playerId   = playerId;
    cmd.sequenceId = seq;
    cmd.inputFlags = flags;
    cmd.x          = x;
    cmd.y          = y;
    cmd.angle      = angle;
    cmd.tickId     = 0;
    return cmd;
}

TEST(PlayerInputSystem, AppliesNewerSequence)
{
    Registry registry;
    EntityId p      = registry.createEntity();
    auto& comp      = registry.emplace<PlayerInputComponent>(p);
    comp.sequenceId = 1;
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem sys(2.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 2, static_cast<std::uint16_t>(InputFlag::MoveUp), 1.0F, 2.0F, 0.5F));

    sys.update(registry, inputs);

    auto& updated = registry.get<PlayerInputComponent>(p);
    EXPECT_EQ(updated.sequenceId, 2);
    EXPECT_FLOAT_EQ(updated.x, 1.0F);
    EXPECT_FLOAT_EQ(updated.y, 2.0F);
    EXPECT_FLOAT_EQ(updated.angle, 0.5F);

    auto& vel = registry.get<VelocityComponent>(p);
    EXPECT_FLOAT_EQ(vel.vx, 0.0F);
    EXPECT_FLOAT_EQ(vel.vy, -2.0F);
}

TEST(PlayerInputSystem, IgnoresStaleSequence)
{
    Registry registry;
    EntityId p      = registry.createEntity();
    auto& comp      = registry.emplace<PlayerInputComponent>(p);
    comp.sequenceId = 5;
    comp.x          = 3.0F;
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem sys(1.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 4, static_cast<std::uint16_t>(InputFlag::MoveLeft), 9.0F, 9.0F, 1.0F));

    sys.update(registry, inputs);

    auto& updated = registry.get<PlayerInputComponent>(p);
    EXPECT_EQ(updated.sequenceId, 5);
    EXPECT_FLOAT_EQ(updated.x, 3.0F);
    auto& vel = registry.get<VelocityComponent>(p);
    EXPECT_FLOAT_EQ(vel.vx, 0.0F);
    EXPECT_FLOAT_EQ(vel.vy, 0.0F);
}

TEST(PlayerInputSystem, NormalizesDiagonalVelocity)
{
    Registry registry;
    EntityId p = registry.createEntity();
    registry.emplace<PlayerInputComponent>(p);
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem sys(4.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(
        makeInput(p, 1, static_cast<std::uint16_t>(InputFlag::MoveUp | InputFlag::MoveRight), 0.0F, 0.0F, 0.0F));

    sys.update(registry, inputs);

    auto& vel = registry.get<VelocityComponent>(p);
    EXPECT_NEAR(vel.vx, 4.0F / std::sqrt(2.0F), 1e-5);
    EXPECT_NEAR(vel.vy, -4.0F / std::sqrt(2.0F), 1e-5);
}

TEST(PlayerInputSystem, SkipsMissingComponentsOrDead)
{
    Registry registry;
    EntityId aliveNoInput = registry.createEntity();
    registry.emplace<VelocityComponent>(aliveNoInput);
    EntityId dead = registry.createEntity();
    registry.emplace<PlayerInputComponent>(dead);
    registry.destroyEntity(dead);

    PlayerInputSystem sys(1.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(aliveNoInput, 1, static_cast<std::uint16_t>(InputFlag::MoveDown)));
    inputs.push_back(makeInput(dead, 2, static_cast<std::uint16_t>(InputFlag::MoveDown)));

    sys.update(registry, inputs);

    auto& vel = registry.get<VelocityComponent>(aliveNoInput);
    EXPECT_FLOAT_EQ(vel.vx, 0.0F);
    EXPECT_FLOAT_EQ(vel.vy, 0.0F);
}

TEST(PlayerInputSystem, CreatesMissileOnFire)
{
    Registry registry;
    EntityId p      = registry.createEntity();
    auto& t         = registry.emplace<TransformComponent>(p);
    t.x             = 10.0F;
    t.y             = 5.0F;
    auto& pic       = registry.emplace<PlayerInputComponent>(p);
    pic.angle       = 0.0F;
    float expectedX = 10.0F;
    float expectedY = 8.0F;

    PlayerInputSystem sys(1.0F, 6.0F, 2.5F, 3);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 1, static_cast<std::uint16_t>(InputFlag::Fire), t.x, t.y, pic.angle));

    sys.update(registry, inputs);

    std::size_t missiles = 0;
    for (EntityId id = 0; id < registry.entityCount(); ++id) {
        if (!registry.isAlive(id))
            continue;
        if (!registry.has<MissileComponent>(id))
            continue;
        ++missiles;
        auto& mt    = registry.get<TransformComponent>(id);
        auto& mv    = registry.get<VelocityComponent>(id);
        auto& owner = registry.get<OwnershipComponent>(id);
        auto& tag   = registry.get<TagComponent>(id);
        auto& mc    = registry.get<MissileComponent>(id);
        EXPECT_FLOAT_EQ(mt.x, expectedX);
        EXPECT_FLOAT_EQ(mt.y, expectedY);
        EXPECT_FLOAT_EQ(mt.rotation, pic.angle);
        EXPECT_FLOAT_EQ(mv.vx, 6.0F);
        EXPECT_FLOAT_EQ(mv.vy, 0.0F);
        EXPECT_EQ(owner.ownerId, p);
        EXPECT_TRUE(tag.hasTag(EntityTag::Projectile));
        EXPECT_EQ(mc.damage, 3);
        EXPECT_FLOAT_EQ(mc.lifetime, 2.5F);
        EXPECT_TRUE(mc.fromPlayer);
    }
    EXPECT_EQ(missiles, 1u);
}

TEST(PlayerInputSystem, MissileMovesWithMovementSystem)
{
    Registry registry;
    EntityId p = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(p);
    t.x        = 0.0F;
    t.y        = 0.0F;
    auto& pic  = registry.emplace<PlayerInputComponent>(p);
    pic.angle  = static_cast<float>(M_PI) / 2.0F;

    PlayerInputSystem sys(1.0F, 4.0F, 1.0F, 1);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 1, static_cast<std::uint16_t>(InputFlag::Fire), 0.0F, 0.0F, pic.angle));
    sys.update(registry, inputs);

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
        EXPECT_NEAR(mt.x, 0.0F, 1e-5);
        EXPECT_NEAR(mt.y, 5.0F, 1e-5);
    }
    EXPECT_EQ(missiles, 1u);
}

TEST(PlayerInputSystem, LatestInputWinsPerPlayer)
{
    Registry registry;
    EntityId p = registry.createEntity();
    registry.emplace<PlayerInputComponent>(p);
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem sys(3.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 2, static_cast<std::uint16_t>(InputFlag::MoveLeft), 1.0F, 1.0F, 0.1F));
    inputs.push_back(makeInput(p, 3, static_cast<std::uint16_t>(InputFlag::MoveRight), 4.0F, 5.0F, 0.9F));

    sys.update(registry, inputs);

    auto& comp = registry.get<PlayerInputComponent>(p);
    EXPECT_EQ(comp.sequenceId, 3);
    EXPECT_FLOAT_EQ(comp.x, 4.0F);
    EXPECT_FLOAT_EQ(comp.y, 5.0F);
    EXPECT_FLOAT_EQ(comp.angle, 0.9F);
    auto& vel = registry.get<VelocityComponent>(p);
    EXPECT_FLOAT_EQ(vel.vx, 3.0F);
    EXPECT_FLOAT_EQ(vel.vy, 0.0F);
}

TEST(PlayerInputSystem, MultiplePlayersIndependent)
{
    Registry registry;
    EntityId p1 = registry.createEntity();
    EntityId p2 = registry.createEntity();
    registry.emplace<PlayerInputComponent>(p1);
    registry.emplace<PlayerInputComponent>(p2);
    registry.emplace<VelocityComponent>(p1);
    registry.emplace<VelocityComponent>(p2);

    PlayerInputSystem sys(2.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p1, 1, static_cast<std::uint16_t>(InputFlag::MoveUp), 1.0F, 2.0F, 0.2F));
    inputs.push_back(
        makeInput(p2, 1, static_cast<std::uint16_t>(InputFlag::MoveDown | InputFlag::MoveLeft), 3.0F, 4.0F, 0.4F));

    sys.update(registry, inputs);

    auto& c1 = registry.get<PlayerInputComponent>(p1);
    auto& c2 = registry.get<PlayerInputComponent>(p2);
    EXPECT_EQ(c1.sequenceId, 1);
    EXPECT_EQ(c2.sequenceId, 1);
    EXPECT_FLOAT_EQ(c1.x, 1.0F);
    EXPECT_FLOAT_EQ(c2.x, 3.0F);
    auto& v1 = registry.get<VelocityComponent>(p1);
    auto& v2 = registry.get<VelocityComponent>(p2);
    EXPECT_FLOAT_EQ(v1.vx, 0.0F);
    EXPECT_FLOAT_EQ(v1.vy, -2.0F);
    EXPECT_NEAR(v2.vx, -2.0F / std::sqrt(2.0F), 1e-5);
    EXPECT_NEAR(v2.vy, 2.0F / std::sqrt(2.0F), 1e-5);
}

TEST(PlayerInputSystem, NoMovementFlagsZeroesVelocity)
{
    Registry registry;
    EntityId p = registry.createEntity();
    registry.emplace<PlayerInputComponent>(p);
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem sys(5.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 1, 0, 0.0F, 0.0F, 0.0F));

    sys.update(registry, inputs);

    auto& vel = registry.get<VelocityComponent>(p);
    EXPECT_FLOAT_EQ(vel.vx, 0.0F);
    EXPECT_FLOAT_EQ(vel.vy, 0.0F);
}

TEST(PlayerInputSystem, InputMovesPlayerWithMovementSystem)
{
    Registry registry;
    EntityId p = registry.createEntity();
    auto& t    = registry.emplace<TransformComponent>(p);
    t.x        = 1.0F;
    t.y        = 2.0F;
    registry.emplace<PlayerInputComponent>(p);
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem inputSys(4.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 1, static_cast<std::uint16_t>(InputFlag::MoveRight)));
    inputSys.update(registry, inputs);

    MovementSystem move;
    move.update(registry, 0.25F);

    auto& nt = registry.get<TransformComponent>(p);
    EXPECT_FLOAT_EQ(nt.x, 2.0F);
    EXPECT_FLOAT_EQ(nt.y, 2.0F);
}

TEST(PlayerInputSystem, StaleInputDoesNotMovePlayer)
{
    Registry registry;
    EntityId p     = registry.createEntity();
    auto& t        = registry.emplace<TransformComponent>(p);
    t.x            = 0.0F;
    t.y            = 0.0F;
    auto& pic      = registry.emplace<PlayerInputComponent>(p);
    pic.sequenceId = 5;
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem inputSys(3.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 4, static_cast<std::uint16_t>(InputFlag::MoveUp)));
    inputSys.update(registry, inputs);

    MovementSystem move;
    move.update(registry, 1.0F);

    auto& nt = registry.get<TransformComponent>(p);
    EXPECT_FLOAT_EQ(nt.x, 0.0F);
    EXPECT_FLOAT_EQ(nt.y, 0.0F);
}

TEST(PlayerInputSystem, LatestInputDeterminesMovementThisFrame)
{
    Registry registry;
    EntityId p = registry.createEntity();
    registry.emplace<TransformComponent>(p);
    registry.emplace<PlayerInputComponent>(p);
    registry.emplace<VelocityComponent>(p);

    PlayerInputSystem inputSys(6.0F);
    std::vector<PlayerCommand> inputs;
    inputs.push_back(makeInput(p, 2, static_cast<std::uint16_t>(InputFlag::MoveLeft)));
    inputs.push_back(makeInput(p, 3, static_cast<std::uint16_t>(InputFlag::MoveDown)));
    inputSys.update(registry, inputs);

    MovementSystem move;
    move.update(registry, 0.5F);

    auto& t = registry.get<TransformComponent>(p);
    EXPECT_FLOAT_EQ(t.x, 0.0F);
    EXPECT_FLOAT_EQ(t.y, 3.0F);
}
