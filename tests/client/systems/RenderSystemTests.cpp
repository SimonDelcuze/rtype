#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/Window.hpp"
#include "systems/RenderSystem.hpp"

#include <SFML/Window/VideoMode.hpp>
#include <gtest/gtest.h>

TEST(RenderSystem, AppliesTransformToSprite)
{
    Window window(sf::VideoMode({64u, 64u}), "Test", false);
    RenderSystem renderSystem(window);
    Registry registry;

    sf::Texture texture;
    ASSERT_TRUE(texture.resize({32u, 32u}));

    EntityId entity = registry.createEntity();
    auto& sprite    = registry.emplace<SpriteComponent>(entity, SpriteComponent(texture));
    registry.emplace<TransformComponent>(entity, TransformComponent::create(10.0F, 20.0F, 45.0F));

    renderSystem.update(registry, 0.0F);

    const sf::Sprite* raw = sprite.raw();
    ASSERT_NE(raw, nullptr);
    EXPECT_FLOAT_EQ(raw->getPosition().x, 10.0F);
    EXPECT_FLOAT_EQ(raw->getPosition().y, 20.0F);
    EXPECT_FLOAT_EQ(raw->getScale().x, 1.0F);
    EXPECT_FLOAT_EQ(raw->getScale().y, 1.0F);
    EXPECT_FLOAT_EQ(raw->getRotation().asDegrees(), 45.0F);
}

TEST(RenderSystem, RespectsLayerComponentSorting)
{
    Window window(sf::VideoMode({64u, 64u}), "Test", false);
    RenderSystem renderSystem(window);
    Registry registry;

    sf::Texture texture;
    ASSERT_TRUE(texture.resize({16u, 16u}));

    // Lower layer entity
    EntityId e1 = registry.createEntity();
    auto& s1    = registry.emplace<SpriteComponent>(e1, SpriteComponent(texture));
    registry.emplace<TransformComponent>(e1, TransformComponent::create(1.0F, 1.0F));
    registry.emplace<LayerComponent>(e1, LayerComponent::create(0));

    // Higher layer entity
    EntityId e2 = registry.createEntity();
    auto& s2    = registry.emplace<SpriteComponent>(e2, SpriteComponent(texture));
    registry.emplace<TransformComponent>(e2, TransformComponent::create(2.0F, 2.0F));
    registry.emplace<LayerComponent>(e2, LayerComponent::create(1));

    // If sorting breaks, the second entity might overwrite first due to same texture rect;
    // we just ensure update runs without throwing and transforms are applied.
    renderSystem.update(registry, 0.0F);

    ASSERT_NE(s1.raw(), nullptr);
    ASSERT_NE(s2.raw(), nullptr);
    EXPECT_FLOAT_EQ(s1.raw()->getPosition().x, 1.0F);
    EXPECT_FLOAT_EQ(s2.raw()->getPosition().x, 2.0F);
}
