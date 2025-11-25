#include "components/BackgroundScrollComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "systems/BackgroundScrollSystem.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

class BackgroundScrollSystemTest : public ::testing::Test
{
  protected:
    Window window{sf::VideoMode({100u, 100u}), "Test", false};
    Registry registry;
    BackgroundScrollSystem system{window};
    std::vector<std::unique_ptr<sf::Texture>> textures;

    sf::Texture& makeTexture(unsigned w, unsigned h)
    {
        auto tex = std::make_unique<sf::Texture>();
        EXPECT_TRUE(tex->resize({w, h}));
        textures.push_back(std::move(tex));
        return *textures.back();
    }

    EntityId createBand(float speedX, float speedY, unsigned texW = 50, unsigned texH = 50)
    {
        EntityId e = registry.createEntity();
        registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
        registry.emplace<BackgroundScrollComponent>(e, BackgroundScrollComponent::create(speedX, speedY, 0.0F, 0.0F));
        auto& tex    = makeTexture(texW, texH);
        auto& sprite = registry.emplace<SpriteComponent>(e);
        sprite.setTexture(tex);
        return e;
    }
};

TEST_F(BackgroundScrollSystemTest, AppliesSpeed)
{
    EntityId e = createBand(-50.0F, -10.0F);
    system.update(registry, 1.0F);
    const auto& t = registry.get<TransformComponent>(e);
    EXPECT_FLOAT_EQ(t.x, -50.0F);
    EXPECT_FLOAT_EQ(t.y, -10.0F);
}

TEST_F(BackgroundScrollSystemTest, AutoScaleAndOffsetsFromWindowAndTexture)
{
    EntityId e   = createBand(-10.0F, 0.0F, 50, 50);
    auto& scroll = registry.get<BackgroundScrollComponent>(e);
    EXPECT_NO_THROW(system.update(registry, 0.0F));
    EXPECT_NE(scroll.resetOffsetX, 0.0F);
}

TEST_F(BackgroundScrollSystemTest, EnsuresCoverageAddsBands)
{
    createBand(-10.0F, 0.0F, 50, 50); // scaled width 100
    system.update(registry, 0.0F);    // first pass computes offsets
    system.update(registry, 0.0F);    // ensure coverage spawns clones

    // window width 100, width band 100 -> coverage loop adds one extra band
    EXPECT_GE(registry.entityCount(), 2u);
}

TEST_F(BackgroundScrollSystemTest, WrapMovesBandToEnd)
{
    EntityId e1 = createBand(-100.0F, 0.0F, 50, 50); // width 100 after scale
    // place a second band manually to be the current maxX
    EntityId e2                            = createBand(-100.0F, 0.0F, 50, 50);
    registry.get<TransformComponent>(e2).x = 100.0F;
    system.update(registry, 0.0F); // compute offsets and coverage

    auto& t1 = registry.get<TransformComponent>(e1);
    auto& t2 = registry.get<TransformComponent>(e2);

    // force t1 to wrap next frame
    t1.x = -100.0F;
    system.update(registry, 0.0F);

    EXPECT_GT(t1.x, t2.x); // moved after the current max
}

TEST_F(BackgroundScrollSystemTest, NextBackgroundAppliedOnWrap)
{
    EntityId e = createBand(-100.0F, 0.0F, 50, 50);
    system.update(registry, 0.0F); // init offsets/scale

    sf::Texture& newTex = makeTexture(25, 25); // different size -> different reset offset
    system.setNextBackground(newTex);

    auto& scroll = registry.get<BackgroundScrollComponent>(e);
    auto& t      = registry.get<TransformComponent>(e);

    // force wrap
    t.x = -scroll.resetOffsetX;
    system.update(registry, 0.0F);

    EXPECT_FLOAT_EQ(scroll.resetOffsetX, 100.0F); // newTex height 25 -> scale 4, width 25*4=100
}

TEST_F(BackgroundScrollSystemTest, IgnoresDeadEntities)
{
    EntityId e = createBand(-10.0F, 0.0F);
    registry.destroyEntity(e);
    EXPECT_NO_THROW(system.update(registry, 1.0F));
}

TEST_F(BackgroundScrollSystemTest, ZeroDeltaTimeKeepsPosition)
{
    EntityId e = createBand(-10.0F, 5.0F);
    system.update(registry, 0.0F);
    const auto& t = registry.get<TransformComponent>(e);
    EXPECT_FLOAT_EQ(t.x, 0.0F);
    EXPECT_FLOAT_EQ(t.y, 0.0F);
}
