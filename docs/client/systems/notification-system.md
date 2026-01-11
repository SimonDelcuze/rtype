# Notification System

## Overview

The `NotificationSystem` is a UI feedback system in the R-Type client that manages the display of temporary notification messages to players. It handles the creation, animation, and lifecycle of toast-style notifications for events such as achievements, connection status changes, errors, and gameplay feedback.

## Purpose and Design Philosophy

Players need clear feedback about game events:

1. **Event Notification**: Inform players of important events
2. **Visual Feedback**: Show temporary, non-intrusive messages
3. **Queue Management**: Handle multiple notifications gracefully
4. **Animation**: Smooth enter/exit animations
5. **Priority Levels**: Different styling for different importance levels

## Class Definition

```cpp
class NotificationSystem : public ISystem
{
  public:
    NotificationSystem(Window& window, FontManager& fonts, EventBus& eventBus);
    
    void initialize() override;
    void update(Registry& registry, float deltaTime) override;
    void cleanup() override;
    
    void showNotification(const std::string& message, NotificationType type = NotificationType::Info,
                          float duration = 3.0F);
    void clearAll();

  private:
    void processQueue();
    void updateActiveNotifications(float deltaTime);
    void renderNotifications();
    
    Window& window_;
    FontManager& fonts_;
    EventBus& eventBus_;
    
    std::queue<NotificationData> pendingQueue_;
    std::vector<ActiveNotification> activeNotifications_;
    
    static constexpr size_t MAX_VISIBLE = 4;
    static constexpr float FADE_DURATION = 0.3F;
};
```

## Notification Types

```cpp
enum class NotificationType
{
    Info,      // Blue - general information
    Success,   // Green - positive feedback
    Warning,   // Yellow - caution
    Error,     // Red - problems/failures
    Achievement // Gold - special accomplishments
};

struct NotificationData
{
    std::string message;
    NotificationType type;
    float duration;
};

struct ActiveNotification
{
    NotificationData data;
    float elapsedTime = 0.0F;
    float alpha = 0.0F;
    float yOffset = 0.0F;
    NotificationState state = NotificationState::Entering;
};
```

## Showing Notifications

```cpp
void NotificationSystem::showNotification(const std::string& message,
                                            NotificationType type,
                                            float duration)
{
    NotificationData data;
    data.message = message;
    data.type = type;
    data.duration = duration;
    
    pendingQueue_.push(data);
}
```

## Update Loop

```cpp
void NotificationSystem::update(Registry& registry, float deltaTime)
{
    // Process pending queue
    processQueue();
    
    // Update active notifications
    updateActiveNotifications(deltaTime);
    
    // Render all active notifications
    renderNotifications();
}

void NotificationSystem::processQueue()
{
    while (!pendingQueue_.empty() && activeNotifications_.size() < MAX_VISIBLE) {
        auto& data = pendingQueue_.front();
        
        ActiveNotification notification;
        notification.data = data;
        notification.state = NotificationState::Entering;
        notification.alpha = 0.0F;
        notification.yOffset = -50.0F;  // Start above final position
        
        activeNotifications_.push_back(notification);
        pendingQueue_.pop();
    }
}
```

## Animation States

```cpp
enum class NotificationState
{
    Entering,   // Fading in, sliding down
    Visible,    // Fully visible, waiting
    Exiting     // Fading out, sliding up
};

void NotificationSystem::updateActiveNotifications(float deltaTime)
{
    for (auto it = activeNotifications_.begin(); it != activeNotifications_.end();) {
        auto& notif = *it;
        notif.elapsedTime += deltaTime;
        
        switch (notif.state) {
            case NotificationState::Entering:
                notif.alpha = std::min(1.0F, notif.elapsedTime / FADE_DURATION);
                notif.yOffset = -50.0F * (1.0F - notif.alpha);
                
                if (notif.alpha >= 1.0F) {
                    notif.state = NotificationState::Visible;
                    notif.elapsedTime = 0.0F;
                }
                break;
                
            case NotificationState::Visible:
                if (notif.elapsedTime >= notif.data.duration) {
                    notif.state = NotificationState::Exiting;
                    notif.elapsedTime = 0.0F;
                }
                break;
                
            case NotificationState::Exiting:
                notif.alpha = 1.0F - std::min(1.0F, notif.elapsedTime / FADE_DURATION);
                notif.yOffset = 50.0F * (1.0F - notif.alpha);
                
                if (notif.alpha <= 0.0F) {
                    it = activeNotifications_.erase(it);
                    continue;
                }
                break;
        }
        
        ++it;
    }
}
```

## Rendering

```cpp
void NotificationSystem::renderNotifications()
{
    float baseY = 100.0F;
    const float spacing = 60.0F;
    
    for (size_t i = 0; i < activeNotifications_.size(); ++i) {
        auto& notif = activeNotifications_[i];
        
        float x = window_.getSize().x - 350;
        float y = baseY + i * spacing + notif.yOffset;
        
        renderNotification(notif, x, y);
    }
}

void NotificationSystem::renderNotification(const ActiveNotification& notif,
                                              float x, float y)
{
    // Get colors based on type
    auto [bgColor, borderColor, iconColor] = getColorsForType(notif.data.type);
    
    // Apply alpha
    bgColor.a = static_cast<uint8_t>(200 * notif.alpha);
    borderColor.a = static_cast<uint8_t>(255 * notif.alpha);
    
    // Draw background
    sf::RectangleShape bg(sf::Vector2f(300, 50));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(bgColor.r, bgColor.g, bgColor.b, bgColor.a));
    bg.setOutlineColor(sf::Color(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
    bg.setOutlineThickness(2);
    window_.draw(bg);
    
    // Draw icon
    std::string icon = getIconForType(notif.data.type);
    drawText(icon, x + 10, y + 15, 18, iconColor);
    
    // Draw message
    Color textColor{230, 230, 240, static_cast<uint8_t>(255 * notif.alpha)};
    drawText(notif.data.message, x + 40, y + 15, 16, textColor);
}
```

## Type Colors and Icons

```cpp
std::tuple<Color, Color, Icon> NotificationSystem::getColorsForType(NotificationType type)
{
    switch (type) {
        case NotificationType::Info:
            return {{50, 100, 180, 200}, {80, 140, 220, 255}, "ℹ"};
            
        case NotificationType::Success:
            return {{50, 150, 80, 200}, {80, 200, 120, 255}, "✓"};
            
        case NotificationType::Warning:
            return {{180, 150, 50, 200}, {220, 200, 80, 255}, "⚠"};
            
        case NotificationType::Error:
            return {{180, 50, 50, 200}, {220, 80, 80, 255}, "✕"};
            
        case NotificationType::Achievement:
            return {{180, 140, 50, 200}, {220, 180, 80, 255}, "★"};
    }
    return {{100, 100, 100, 200}, {150, 150, 150, 255}, "?"};
}
```

## Event Bus Integration

```cpp
void NotificationSystem::initialize()
{
    // Subscribe to relevant events
    eventBus_.subscribe<ConnectionSuccessEvent>([this](const auto&) {
        showNotification("Connected to server", NotificationType::Success);
    });
    
    eventBus_.subscribe<ConnectionFailedEvent>([this](const auto& e) {
        showNotification("Connection failed: " + e.reason, NotificationType::Error);
    });
    
    eventBus_.subscribe<AchievementUnlockedEvent>([this](const auto& e) {
        showNotification("Achievement: " + e.name, NotificationType::Achievement, 5.0F);
    });
    
    eventBus_.subscribe<PlayerJoinedEvent>([this](const auto& e) {
        showNotification(e.playerName + " joined the game", NotificationType::Info);
    });
}
```

## Usage Examples

```cpp
// Direct usage
notificationSystem_.showNotification("Game saved", NotificationType::Success);

// Error with longer duration
notificationSystem_.showNotification(
    "Failed to load level data",
    NotificationType::Error,
    5.0F  // 5 seconds
);

// Achievement
notificationSystem_.showNotification(
    "First Blood!",
    NotificationType::Achievement,
    4.0F
);
```

## Best Practices

1. **Concise Messages**: Keep notifications short and clear
2. **Appropriate Duration**: Longer for important, shorter for minor
3. **Don't Spam**: Rate-limit repetitive notifications
4. **Consistent Styling**: Use type colors consistently
5. **Accessibility**: Ensure sufficient contrast

## Related Components

- [NotificationData](../components/notification-data.md) - Notification structure

## Related Systems

- [HUDSystem](hud-system.md) - Overall HUD management
- [RenderSystem](render-system.md) - Drawing infrastructure
