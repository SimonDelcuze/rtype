#pragma once

#include "components/CameraComponent.hpp"
#include "ecs/Registry.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <limits>

class CameraSystem
{
  public:
    explicit CameraSystem(sf::RenderWindow& window);

    void update(Registry& registry, float deltaTime);

    sf::View& getView();

    const sf::View& getView() const;

    void setWorldBounds(float left, float top, float width, float height);

    void setWorldBoundsEnabled(bool enabled);

    EntityId getActiveCamera() const;

  private:
    EntityId findActiveCamera(Registry& registry);

    void applyCamera(const CameraComponent& camera);

    void clampToWorldBounds(CameraComponent& camera);

    void updateCameraFollow(Registry& registry, CameraComponent& camera, float deltaTime);

    sf::RenderWindow& window_;
    sf::View view_;
    EntityId activeCameraId_{std::numeric_limits<EntityId>::max()};
    sf::Vector2f baseViewSize_;
    bool worldBoundsEnabled_{false};
    float worldLeft_{0.0F};
    float worldTop_{0.0F};
    float worldWidth_{0.0F};
    float worldHeight_{0.0F};
};
