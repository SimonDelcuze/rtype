# RenderSystem

## Overview

The `RenderSystem` is the core visual presentation system in the R-Type client responsible for drawing all visible entities to the screen. It manages the rendering pipeline, handles layer-based sorting for proper draw order, applies camera transforms, and coordinates the display of sprites, text, shapes, and UI elements.

## Purpose and Design Philosophy

The rendering system serves as the single point of visual output:

1. **Layer Sorting**: Ensures correct visual depth ordering
2. **Camera Transformation**: Applies viewport transforms to world entities
3. **Batch Optimization**: Groups similar draw calls for performance
4. **Component Integration**: Renders multiple component types (sprites, boxes, text)
5. **Frame Management**: Handles frame clearing and display

Following the ECS pattern, the system contains no entity data—it only reads components and produces visual output.

## Class Definition

```cpp
class RenderSystem : public ISystem
{
  public:
    explicit RenderSystem(Window& window);

    void update(Registry& registry, float deltaTime) override;

  private:
    Window& window_;
};
```

## Constructor

```cpp
explicit RenderSystem(Window& window);
```

**Parameters:**
- `window`: Reference to the game window for rendering operations

**Example:**
```cpp
RenderSystem renderSystem(window);
scheduler.addSystem(&renderSystem);
```

## Main Update Method

```cpp
void update(Registry& registry, float deltaTime) override
{
    // Clear the frame
    window_.clear(Color{0, 0, 0, 255});
    
    // Apply camera transform
    applyCamera(registry);
    
    // Collect and sort renderables
    auto renderables = collectRenderables(registry);
    std::sort(renderables.begin(), renderables.end(), byLayer);
    
    // Render in sorted order
    for (const auto& renderable : renderables) {
        render(registry, renderable);
    }
    
    // Reset view for UI
    window_.resetView();
    
    // Render UI layer without camera transform
    renderUI(registry);
    
    // Display the frame
    window_.display();
}
```

## Rendering Pipeline

### 1. Collecting Renderables

```cpp
struct Renderable
{
    EntityId entity;
    int layer;
    RenderType type;
};

std::vector<Renderable> RenderSystem::collectRenderables(Registry& registry)
{
    std::vector<Renderable> result;
    
    // Collect sprites
    auto spriteView = registry.view<SpriteComponent, TransformComponent>();
    for (auto entity : spriteView) {
        Renderable r;
        r.entity = entity;
        r.layer = getLayer(registry, entity);
        r.type = RenderType::Sprite;
        result.push_back(r);
    }
    
    // Collect boxes
    auto boxView = registry.view<BoxComponent, TransformComponent>();
    for (auto entity : boxView) {
        Renderable r;
        r.entity = entity;
        r.layer = getLayer(registry, entity);
        r.type = RenderType::Box;
        result.push_back(r);
    }
    
    // Collect text
    auto textView = registry.view<TextComponent, TransformComponent>();
    for (auto entity : textView) {
        Renderable r;
        r.entity = entity;
        r.layer = getLayer(registry, entity);
        r.type = RenderType::Text;
        result.push_back(r);
    }
    
    return result;
}
```

### 2. Layer Sorting

```cpp
int RenderSystem::getLayer(Registry& registry, EntityId entity)
{
    if (registry.hasComponent<LayerComponent>(entity)) {
        return registry.getComponent<LayerComponent>(entity).layer;
    }
    return RenderLayer::Entities;  // Default layer
}

bool RenderSystem::byLayer(const Renderable& a, const Renderable& b)
{
    return a.layer < b.layer;  // Lower layers render first (behind)
}
```

### 3. Camera Application

```cpp
void RenderSystem::applyCamera(Registry& registry)
{
    auto cameraView = registry.view<CameraComponent>();
    
    for (auto entity : cameraView) {
        auto& camera = cameraView.get<CameraComponent>(entity);
        
        if (!camera.active) continue;
        
        sf::View view = window_.getDefaultView();
        
        // Set view center
        view.setCenter(camera.x, camera.y);
        
        // Apply zoom
        view.setSize(
            window_.getSize().x / camera.zoom,
            window_.getSize().y / camera.zoom
        );
        
        // Apply rotation
        view.setRotation(camera.rotation);
        
        window_.setView(view);
        break;  // Only one active camera
    }
}
```

### 4. Entity Rendering

```cpp
void RenderSystem::render(Registry& registry, const Renderable& renderable)
{
    switch (renderable.type) {
        case RenderType::Sprite:
            renderSprite(registry, renderable.entity);
            break;
        case RenderType::Box:
            renderBox(registry, renderable.entity);
            break;
        case RenderType::Text:
            renderText(registry, renderable.entity);
            break;
    }
}
```

## Sprite Rendering

```cpp
void RenderSystem::renderSprite(Registry& registry, EntityId entity)
{
    auto& sprite = registry.getComponent<SpriteComponent>(entity);
    auto& transform = registry.getComponent<TransformComponent>(entity);
    
    // Get texture
    auto* texture = textureManager_.get(sprite.textureId);
    if (!texture) return;
    
    // Create drawable sprite
    sf::Sprite sfSprite(*texture);
    
    // Set texture rect for sprite sheets
    sfSprite.setTextureRect(sf::IntRect(
        sprite.frameX * sprite.frameWidth,
        sprite.frameY * sprite.frameHeight,
        sprite.frameWidth,
        sprite.frameHeight
    ));
    
    // Apply transform
    sfSprite.setPosition(transform.x, transform.y);
    sfSprite.setRotation(transform.rotation);
    sfSprite.setScale(sprite.scaleX, sprite.scaleY);
    
    // Handle flipping
    if (sprite.flipHorizontal) {
        sfSprite.setScale(-sfSprite.getScale().x, sfSprite.getScale().y);
        sfSprite.setOrigin(sprite.frameWidth, 0);
    }
    
    // Apply color filter
    sfSprite.setColor(sf::Color(
        sprite.color.r, sprite.color.g, sprite.color.b, sprite.color.a
    ));
    
    window_.draw(sfSprite);
}
```

## Box Rendering

```cpp
void RenderSystem::renderBox(Registry& registry, EntityId entity)
{
    auto& box = registry.getComponent<BoxComponent>(entity);
    auto& transform = registry.getComponent<TransformComponent>(entity);
    
    // Check for focus state
    bool isFocused = false;
    if (registry.hasComponent<FocusableComponent>(entity)) {
        isFocused = registry.getComponent<FocusableComponent>(entity).focused;
    }
    
    // Check for hover state
    bool isHovered = false;
    if (registry.hasComponent<ButtonComponent>(entity)) {
        isHovered = registry.getComponent<ButtonComponent>(entity).hovered;
    }
    
    // Create rectangle
    sf::RectangleShape rect(sf::Vector2f(box.width, box.height));
    rect.setPosition(transform.x, transform.y);
    
    // Apply colors
    Color fillColor = box.fillColor;
    Color outlineColor = isFocused || isHovered ? box.focusColor : box.outlineColor;
    
    rect.setFillColor(sf::Color(fillColor.r, fillColor.g, fillColor.b, fillColor.a));
    rect.setOutlineColor(sf::Color(outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a));
    rect.setOutlineThickness(box.outlineThickness);
    
    window_.draw(rect);
}
```

## Text Rendering

```cpp
void RenderSystem::renderText(Registry& registry, EntityId entity)
{
    auto& text = registry.getComponent<TextComponent>(entity);
    auto& transform = registry.getComponent<TransformComponent>(entity);
    
    // Check for input field (may show placeholder or masked text)
    std::string displayText = text.content;
    if (registry.hasComponent<InputFieldComponent>(entity)) {
        auto& input = registry.getComponent<InputFieldComponent>(entity);
        
        if (input.value.empty() && !input.focused) {
            displayText = input.placeholder;
        } else if (input.passwordField) {
            displayText = std::string(input.value.length(), '*');
        } else {
            displayText = input.value;
        }
    }
    
    sf::Text sfText;
    sfText.setFont(*fontManager_.get(text.fontId));
    sfText.setString(displayText);
    sfText.setCharacterSize(text.fontSize);
    sfText.setFillColor(sf::Color(text.color.r, text.color.g, text.color.b, text.color.a));
    sfText.setPosition(transform.x, transform.y);
    
    window_.draw(sfText);
}
```

## UI Layer Rendering

UI elements render without camera transform:

```cpp
void RenderSystem::renderUI(Registry& registry)
{
    // Reset to default view (no camera effects)
    window_.setView(window_.getDefaultView());
    
    // Collect UI renderables only
    auto renderables = collectRenderables(registry);
    
    // Filter to UI layers only
    renderables.erase(
        std::remove_if(renderables.begin(), renderables.end(),
            [](const Renderable& r) { return r.layer < RenderLayer::UI; }),
        renderables.end()
    );
    
    // Sort and render
    std::sort(renderables.begin(), renderables.end(), byLayer);
    for (const auto& r : renderables) {
        render(registry, r);
    }
}
```

## Special Effects

### Color Filter Application

```cpp
void RenderSystem::applyColorFilter(sf::Sprite& sprite, const ColorFilter& filter)
{
    if (filter.type == ColorFilter::Type::None) return;
    
    switch (filter.type) {
        case ColorFilter::Type::Invincibility:
            // Flashing white
            float flash = std::sin(m_time * 20.0F) * 0.5F + 0.5F;
            sprite.setColor(sf::Color(
                255,
                static_cast<uint8_t>(255 * flash),
                static_cast<uint8_t>(255 * flash),
                255
            ));
            break;
            
        case ColorFilter::Type::Damage:
            // Red tint
            sprite.setColor(sf::Color(255, 100, 100, 255));
            break;
    }
}
```

### Particle Effects

```cpp
void RenderSystem::renderParticles(Registry& registry)
{
    auto view = registry.view<ParticleEmitterComponent, TransformComponent>();
    
    for (auto entity : view) {
        auto& emitter = view.get<ParticleEmitterComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);
        
        for (const auto& particle : emitter.particles) {
            sf::CircleShape circle(particle.size);
            circle.setPosition(transform.x + particle.x, transform.y + particle.y);
            circle.setFillColor(sf::Color(
                particle.color.r,
                particle.color.g,
                particle.color.b,
                static_cast<uint8_t>(255 * particle.alpha)
            ));
            window_.draw(circle);
        }
    }
}
```

## Render Order Summary

```
1. Clear screen to black
2. Apply camera transform
3. Render Background layer (-100)
4. Render Midground layer (-50)
5. Render Entities layer (0)
6. Render Effects layer (50)
7. Reset view (no camera)
8. Render UI layer (100)
9. Render HUD layer (150)
10. Render Debug layer (200)
11. Display frame
```

## Performance Optimizations

```cpp
// Batch sprites by texture
void RenderSystem::renderSpriteBatched(Registry& registry)
{
    std::unordered_map<std::string, std::vector<EntityId>> byTexture;
    
    auto view = registry.view<SpriteComponent, TransformComponent, LayerComponent>();
    for (auto entity : view) {
        auto& sprite = view.get<SpriteComponent>(entity);
        byTexture[sprite.textureId].push_back(entity);
    }
    
    // Render each texture batch
    for (auto& [textureId, entities] : byTexture) {
        // Sort by layer within batch
        std::sort(entities.begin(), entities.end(), [&](EntityId a, EntityId b) {
            return view.get<LayerComponent>(a).layer < 
                   view.get<LayerComponent>(b).layer;
        });
        
        // Draw all sprites with same texture
        for (auto entity : entities) {
            renderSprite(registry, entity);
        }
    }
}
```

## Best Practices

1. **Layer Consistency**: Always assign layers explicitly
2. **Camera Awareness**: Know which entities are affected by camera
3. **Culling**: Skip off-screen entities for large scenes
4. **Texture Atlasing**: Combine sprites to reduce draw calls
5. **Minimize State Changes**: Sort by texture within layers

## Related Components

- [SpriteComponent](../components/sprite-component.md) - Sprite visuals
- [BoxComponent](../components/box-component.md) - Rectangular shapes
- [TextComponent](../components/text-component.md) - Text display
- [LayerComponent](../components/layer-component.md) - Render ordering
- [CameraComponent](../components/camera-component.md) - Viewport control

## Related Systems

- [CameraSystem](camera-system.md) - Camera updates
- [AnimationSystem](animation-system.md) - Frame updates for sprites
