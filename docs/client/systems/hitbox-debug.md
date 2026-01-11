# HitboxDebugSystem

## Overview

The `HitboxDebugSystem` is a development and debugging system in the R-Type client that visualizes collision hitboxes for all entities with collider components. It renders transparent overlays showing the exact collision boundaries, making it invaluable for debugging collision detection issues, balancing hitbox sizes, and ensuring gameplay fairness.

## Purpose and Design Philosophy

During development, collision visualization is essential:

1. **Visual Debugging**: See exact collision boundaries
2. **Hitbox Tuning**: Verify hitbox sizes match visual sprites
3. **Collision Testing**: Debug unexpected collision behavior
4. **Player Fairness**: Ensure player hitbox is appropriately sized
5. **Production Toggle**: Can be disabled for release builds

## Class Definition

```cpp
class HitboxDebugSystem : public ISystem
{
  public:
    explicit HitboxDebugSystem(Window& window);
    
    void initialize() override {}
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override {}
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    void toggle() { enabled_ = !enabled_; }

  private:
    Window& window_;
    bool enabled_ = false;
    
    void renderBoxCollider(const ColliderComponent& collider, 
                           const TransformComponent& transform);
    void renderCircleCollider(const ColliderComponent& collider,
                               const TransformComponent& transform);
    void renderPolygonCollider(const ColliderComponent& collider,
                                const TransformComponent& transform);
};
```

## Main Update Loop

```cpp
void HitboxDebugSystem::update(Registry& registry, float deltaTime)
{
    if (!enabled_) return;
    
    auto view = registry.view<ColliderComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& collider = view.get<ColliderComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        if (!collider.isActive) continue;
        
        // Choose render method based on shape
        switch (collider.shape) {
            case ColliderComponent::Shape::Box:
                renderBoxCollider(collider, transform);
                break;
            case ColliderComponent::Shape::Circle:
                renderCircleCollider(collider, transform);
                break;
            case ColliderComponent::Shape::Polygon:
                renderPolygonCollider(collider, transform);
                break;
        }
    }
}
```

## Collider Rendering

### Box Collider

```cpp
void HitboxDebugSystem::renderBoxCollider(const ColliderComponent& collider,
                                            const TransformComponent& transform)
{
    float x = transform.x + collider.offsetX;
    float y = transform.y + collider.offsetY;
    
    sf::RectangleShape rect(sf::Vector2f(collider.width, collider.height));
    rect.setPosition(x, y);
    
    // Semi-transparent fill
    rect.setFillColor(sf::Color(255, 0, 0, 50));
    
    // Solid outline
    rect.setOutlineColor(sf::Color(255, 0, 0, 200));
    rect.setOutlineThickness(1.0F);
    
    window_.draw(rect);
}
```

### Circle Collider

```cpp
void HitboxDebugSystem::renderCircleCollider(const ColliderComponent& collider,
                                               const TransformComponent& transform)
{
    float x = transform.x + collider.offsetX;
    float y = transform.y + collider.offsetY;
    
    sf::CircleShape circle(collider.radius);
    circle.setPosition(x - collider.radius, y - collider.radius);
    
    // Semi-transparent fill
    circle.setFillColor(sf::Color(0, 255, 0, 50));
    
    // Solid outline
    circle.setOutlineColor(sf::Color(0, 255, 0, 200));
    circle.setOutlineThickness(1.0F);
    
    window_.draw(circle);
}
```

### Polygon Collider

```cpp
void HitboxDebugSystem::renderPolygonCollider(const ColliderComponent& collider,
                                                const TransformComponent& transform)
{
    if (collider.points.empty()) return;
    
    sf::ConvexShape polygon(collider.points.size());
    
    for (size_t i = 0; i < collider.points.size(); ++i) {
        float px = transform.x + collider.offsetX + collider.points[i][0];
        float py = transform.y + collider.offsetY + collider.points[i][1];
        polygon.setPoint(i, sf::Vector2f(px, py));
    }
    
    // Semi-transparent fill
    polygon.setFillColor(sf::Color(0, 0, 255, 50));
    
    // Solid outline
    polygon.setOutlineColor(sf::Color(0, 0, 255, 200));
    polygon.setOutlineThickness(1.0F);
    
    window_.draw(polygon);
}
```

## Color Coding

Different entity types can use different colors:

```cpp
Color HitboxDebugSystem::getColorForEntity(Registry& registry, EntityId entity)
{
    // Player - green (friendly)
    if (registry.hasComponent<PlayerComponent>(entity)) {
        return Color{0, 255, 0, 128};
    }
    
    // Enemy - red (hostile)
    if (registry.hasComponent<EnemyComponent>(entity)) {
        return Color{255, 0, 0, 128};
    }
    
    // Player projectile - cyan
    if (registry.hasComponent<ProjectileComponent>(entity)) {
        auto& proj = registry.getComponent<ProjectileComponent>(entity);
        if (proj.isPlayerOwned) {
            return Color{0, 255, 255, 128};
        } else {
            return Color{255, 165, 0, 128};  // Orange for enemy projectiles
        }
    }
    
    // Power-up - yellow
    if (registry.hasComponent<PowerUpComponent>(entity)) {
        return Color{255, 255, 0, 128};
    }
    
    // Default - white
    return Color{255, 255, 255, 128};
}
```

## Toggle Integration

```cpp
void GameLoop::handleDebugInput(const Event& event)
{
    if (event.type == EventType::KeyPressed) {
        if (event.key.code == Key::F1) {
            hitboxDebugSystem_.toggle();
        }
    }
}
```

## Enhanced Debug Information

```cpp
void HitboxDebugSystem::renderEnhanced(Registry& registry, EntityId entity,
                                         const ColliderComponent& collider,
                                         const TransformComponent& transform)
{
    // Render basic hitbox
    renderCollider(collider, transform);
    
    // Add label with entity type
    std::string label = getEntityLabel(registry, entity);
    float labelX = transform.x + collider.offsetX;
    float labelY = transform.y + collider.offsetY - 15;
    
    drawText(label, labelX, labelY, 10, Color{255, 255, 255, 200});
    
    // Show collision layer info
    if (registry.hasComponent<CollisionLayerComponent>(entity)) {
        auto& layer = registry.getComponent<CollisionLayerComponent>(entity);
        std::string layerInfo = "Layer: " + std::to_string(layer.layer);
        drawText(layerInfo, labelX, labelY - 12, 8, Color{200, 200, 200, 200});
    }
}

std::string HitboxDebugSystem::getEntityLabel(Registry& registry, EntityId entity)
{
    if (registry.hasComponent<PlayerComponent>(entity)) return "PLAYER";
    if (registry.hasComponent<BossComponent>(entity)) {
        auto& boss = registry.getComponent<BossComponent>(entity);
        return boss.name;
    }
    if (registry.hasComponent<EnemyComponent>(entity)) return "ENEMY";
    if (registry.hasComponent<ProjectileComponent>(entity)) return "PROJ";
    if (registry.hasComponent<PowerUpComponent>(entity)) return "POWERUP";
    return "?";
}
```

## Performance Considerations

```cpp
void HitboxDebugSystem::update(Registry& registry, float deltaTime)
{
    if (!enabled_) return;
    
    // Skip if too many entities (prevent lag in stress tests)
    auto view = registry.view<ColliderComponent, TransformComponent>();
    size_t entityCount = std::distance(view.begin(), view.end());
    
    if (entityCount > MAX_DEBUG_ENTITIES) {
        // Only render player and bosses
        renderCriticalOnly(registry);
        drawWarning("Too many entities - limited debug view");
        return;
    }
    
    // Normal rendering
    for (auto entity : view) {
        render(registry, entity);
    }
}
```

## Best Practices

1. **Disable in Release**: Toggle off for production builds
2. **Color Coding**: Use consistent colors for entity types
3. **Performance Limit**: Cap rendered hitboxes in stress scenarios
4. **Hotkey Toggle**: Use function keys for quick toggling
5. **Layer Visualization**: Consider showing collision layers

## Related Components

- [ColliderComponent](../components/collider-component.md) - Collision data
- [TransformComponent](../server/components/transform-component.md) - Position data

## Related Systems

- [RenderSystem](render-system.md) - Main rendering (debug draws after)
- [CollisionSystem](../server/collision-system.md) - Uses these hitboxes
