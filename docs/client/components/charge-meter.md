# ChargeMeterComponent

## Overview

The `ChargeMeterComponent` is a specialized UI component in the R-Type client that tracks and displays the charge progress for the player's charged shot mechanic. This component is fundamental to R-Type's signature gameplay feature—the ability to hold the fire button to charge a powerful beam weapon that deals significantly more damage than standard shots.

## Purpose and Design Philosophy

The charged shot is a core mechanic in R-Type games, adding strategic depth to combat:

1. **Risk/Reward**: Players must balance charging time with exposure to enemy fire
2. **Visual Feedback**: Players need clear indication of their current charge level
3. **Timing Mastery**: Skilled players learn optimal charge durations
4. **Resource Management**: Charging requires staying alive and not firing

The `ChargeMeterComponent` provides the data backbone for this visual feedback system, storing the current charge progress value that is then rendered by the HUD system.

## Component Structure

```cpp
struct ChargeMeterComponent
{
    float progress = 0.0F;
};
```

### Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `progress` | `float` | `0.0F` | The current charge level, ranging from 0.0 (empty) to 1.0 (fully charged). Values outside this range may indicate overcharge states. |

## Reset Value Support

The component implements proper ECS reset functionality:

```cpp
template <> inline void resetValue<ChargeMeterComponent>(ChargeMeterComponent& value)
{
    value = ChargeMeterComponent{};
}
```

This ensures the charge meter resets to zero when component pooling recycles the component.

## Charge Level Semantics

The `progress` value has semantic meaning at different ranges:

| Progress Range | Meaning | Visual Representation |
|----------------|---------|----------------------|
| `0.0` | No charge | Empty meter |
| `0.0 - 0.25` | Level 1 (Weak) | 25% filled |
| `0.25 - 0.50` | Level 2 (Medium) | 50% filled |
| `0.50 - 0.75` | Level 3 (Strong) | 75% filled |
| `0.75 - 1.0` | Level 4 (Maximum) | Full meter |
| `1.0` | Fully charged | Glowing/pulsing effect |
| `> 1.0` | Overcharge (optional) | Special effect |

## Usage Patterns

### Creating the Charge Meter HUD Element

```cpp
Entity createChargeMeterUI(Registry& registry)
{
    auto entity = registry.createEntity();
    
    // Position at bottom-left of screen
    registry.addComponent(entity, TransformComponent::create(50.0F, SCREEN_HEIGHT - 80.0F));
    
    // Charge meter data
    registry.addComponent(entity, ChargeMeterComponent{});
    
    // Visual background bar
    registry.addComponent(entity, BoxComponent::create(
        200.0F, 20.0F,
        Color{30, 30, 40, 200},
        Color{80, 80, 100, 255}
    ));
    
    // HUD layer
    registry.addComponent(entity, LayerComponent::create(RenderLayer::HUD));
    
    return entity;
}
```

### Updating Charge Progress

The charge is typically updated based on player input:

```cpp
void ChargeSystem::update(Registry& registry, float deltaTime)
{
    auto playerView = registry.view<PlayerInputComponent, WeaponComponent>();
    auto meterView = registry.view<ChargeMeterComponent>();
    
    for (auto playerEntity : playerView) {
        auto& input = playerView.get<PlayerInputComponent>(playerEntity);
        auto& weapon = playerView.get<WeaponComponent>(playerEntity);
        
        // Check if fire button is held
        if (input.fireHeld) {
            weapon.chargeTime += deltaTime;
            float progress = std::min(1.0F, weapon.chargeTime / weapon.maxChargeTime);
            
            // Update all charge meter displays
            for (auto meterEntity : meterView) {
                auto& meter = meterView.get<ChargeMeterComponent>(meterEntity);
                meter.progress = progress;
            }
        } else if (weapon.chargeTime > 0.0F) {
            // Fire button released - fire charged shot
            fireChargedShot(registry, playerEntity, weapon.chargeTime);
            weapon.chargeTime = 0.0F;
            
            // Reset meter
            for (auto meterEntity : meterView) {
                auto& meter = meterView.get<ChargeMeterComponent>(meterEntity);
                meter.progress = 0.0F;
            }
        }
    }
}
```

### Rendering the Charge Meter

The HUD system renders the charge meter based on progress:

```cpp
void HUDSystem::renderChargeMeter(const ChargeMeterComponent& meter,
                                   const TransformComponent& transform,
                                   const BoxComponent& background)
{
    // Draw background bar
    drawRect(transform.x, transform.y, background.width, background.height,
             background.fillColor, background.outlineColor);
    
    // Calculate fill width
    float fillWidth = background.width * meter.progress;
    
    // Determine fill color based on charge level
    Color fillColor = getChargeColor(meter.progress);
    
    // Draw filled portion
    if (fillWidth > 0.0F) {
        drawRect(transform.x + 2, transform.y + 2,
                 fillWidth - 4, background.height - 4,
                 fillColor, Color{0, 0, 0, 0});
    }
    
    // Add glow effect at full charge
    if (meter.progress >= 1.0F) {
        float glowIntensity = 0.5F + 0.5F * std::sin(m_time * 10.0F);
        drawGlow(transform.x, transform.y, background.width, background.height,
                 Color{255, 255, 100, static_cast<uint8_t>(glowIntensity * 128)});
    }
}

Color HUDSystem::getChargeColor(float progress)
{
    if (progress < 0.25F) {
        return Color{100, 150, 255};   // Blue - weak
    } else if (progress < 0.50F) {
        return Color{100, 255, 150};   // Green - medium
    } else if (progress < 0.75F) {
        return Color{255, 200, 100};   // Orange - strong
    } else {
        return Color{255, 100, 100};   // Red - maximum
    }
}
```

### Audio Feedback Integration

Charge level affects sound effects:

```cpp
void AudioSystem::updateChargeSound(const ChargeMeterComponent& meter)
{
    if (meter.progress > 0.0F && meter.progress < 1.0F) {
        // Play charging sound that increases in pitch
        float pitch = 0.8F + meter.progress * 0.4F;
        m_chargeSound.setPitch(pitch);
        
        if (!m_chargeSound.isPlaying()) {
            m_chargeSound.play();
        }
    } else if (meter.progress >= 1.0F) {
        // Play full charge loop
        if (!m_fullChargeSound.isPlaying()) {
            m_chargeSound.stop();
            m_fullChargeSound.play();
        }
    } else {
        // No charge - stop sounds
        m_chargeSound.stop();
        m_fullChargeSound.stop();
    }
}
```

### Animation Integration

Charge level can trigger visual effects on the player ship:

```cpp
void updateChargeVisuals(Registry& registry, Entity playerEntity, float progress)
{
    if (registry.hasComponent<SpriteComponent>(playerEntity)) {
        auto& sprite = registry.getComponent<SpriteComponent>(playerEntity);
        
        if (progress >= 1.0F) {
            // Full charge - set glowing sprite
            sprite.textureId = "player_charged";
        } else if (progress > 0.5F) {
            // High charge - set charging sprite
            sprite.textureId = "player_charging";
        } else {
            // Normal or low charge - normal sprite
            sprite.textureId = "player";
        }
    }
}
```

## Charge Shot Damage Scaling

Different charge levels produce different shot types:

```cpp
ShotType getShotTypeFromCharge(float progress)
{
    if (progress < 0.25F) {
        return ShotType::Normal;        // Standard shot
    } else if (progress < 0.50F) {
        return ShotType::ChargeSmall;   // Small charged shot
    } else if (progress < 0.75F) {
        return ShotType::ChargeMedium;  // Medium charged shot
    } else if (progress < 1.0F) {
        return ShotType::ChargeLarge;   // Large charged shot
    } else {
        return ShotType::ChargeMax;     // Maximum beam
    }
}

float getDamageMultiplier(float progress)
{
    // Exponential scaling rewards full charges
    return 1.0F + (progress * progress) * 4.0F;
    // progress 0.00 -> 1.0x
    // progress 0.25 -> 1.25x
    // progress 0.50 -> 2.0x
    // progress 0.75 -> 3.25x
    // progress 1.00 -> 5.0x
}
```

## Visual Design Guidelines

### Meter Appearance

```
┌────────────────────────────────────────────────────┐
│ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░ │  <- 50% charge
└────────────────────────────────────────────────────┘
  [==============             ] CHARGE
```

### Color Progression

```cpp
// Gradient from blue (empty) to red (full)
Color interpolateChargeColor(float progress)
{
    if (progress < 0.5F) {
        // Blue to green
        float t = progress * 2.0F;
        return Color{
            static_cast<uint8_t>(100 * (1 - t)),
            static_cast<uint8_t>(150 + 105 * t),
            static_cast<uint8_t>(255 - 105 * t)
        };
    } else {
        // Green to red
        float t = (progress - 0.5F) * 2.0F;
        return Color{
            static_cast<uint8_t>(100 + 155 * t),
            static_cast<uint8_t>(255 * (1 - t)),
            static_cast<uint8_t>(150 * (1 - t))
        };
    }
}
```

## Best Practices

1. **Smooth Updates**: Update progress smoothly, not in discrete jumps
2. **Clear Visual Feedback**: Make charge levels clearly distinguishable
3. **Audio Sync**: Keep audio feedback synchronized with visual
4. **Reset on Death**: Clear charge when player dies or respawns
5. **Network Sync**: In multiplayer, charge is typically client-predicted

## Multiplayer Considerations

```cpp
// Charge is client-predicted - no need to sync every frame
// Only sync when shot is fired
struct ChargedShotPacket
{
    uint32_t playerId;
    float chargeLevel;  // Sent for validation
    float posX, posY;
    float angle;
};
```

## Related Components

- [PlayerInputComponent](../server/components/player-input-component.md) - Input that affects charging
- [WeaponComponent](../server/components/weapon-component.md) - Weapon state and charge time
- [BoxComponent](box-component.md) - Visual background for meter

## Related Systems

- [HUDSystem](../systems/hud-system.md) - Renders the charge meter
- [AudioSystem](../systems/audio-system.md) - Charge sound effects
- [InputSystem](input-pipeline.md) - Detects fire button held
