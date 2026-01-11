# ConnectionMenu

## Overview

The `ConnectionMenu` is the initial entry point for multiplayer gameplay in the R-Type client. It provides an interface for players to enter the lobby server's IP address and port, initiate connection, and handle connection failures. This menu serves as the gateway between single-player mode and the multiplayer experience.

## Purpose and Responsibilities

The ConnectionMenu handles:

1. **Server Address Input**: IP address and port number entry
2. **Connection Initiation**: Triggering connection to lobby server
3. **Connection Status**: Visual feedback during connection attempt
4. **Error Display**: Showing connection failure reasons
5. **Navigation**: Access to settings and exit functionality

## Class Structure

```cpp
class ConnectionMenu : public IMenu
{
  public:
    struct Result
    {
        bool connected     = false;
        bool openSettings  = false;
        bool exitRequested = false;
        std::string ip;
        std::string port;
    };

    ConnectionMenu(FontManager& fonts, TextureManager& textures, 
                   std::string initialError = "");

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const;
    void setError(Registry& registry, const std::string& message);
    void reset();
};
```

## UI Layout

```
┌────────────────────────────────────────────────┐
│                                                │
│              R - T Y P E                       │
│           Multiplayer Lobby                    │
│                                                │
│   ┌──────────────────────────────────────┐    │
│   │ Server IP:  [127.0.0.1          ]    │    │
│   └──────────────────────────────────────┘    │
│                                                │
│   ┌──────────────────────────────────────┐    │
│   │ Port:       [50010              ]    │    │
│   └──────────────────────────────────────┘    │
│                                                │
│        [ Connect ]     [ Settings ]           │
│                                                │
│   ⚠ Connection failed: timeout                │
│      (if error present)                        │
│                                                │
│   Connecting to server...                     │
│   (animated dots during connection)            │
│                                                │
│                      [ Exit ]                  │
└────────────────────────────────────────────────┘
```

## Menu Creation

```cpp
void ConnectionMenu::create(Registry& registry)
{
    const float centerX = SCREEN_WIDTH / 2 - 150;
    const float startY = 150;
    
    // Title
    auto title = registry.createEntity();
    registry.addComponent(title, TransformComponent::create(SCREEN_WIDTH / 2, 80));
    registry.addComponent(title, TextComponent::create("R-TYPE", 48, 
        Color{255, 255, 255}));
    registry.addComponent(title, LayerComponent::create(RenderLayer::UI));
    
    auto subtitle = registry.createEntity();
    registry.addComponent(subtitle, TransformComponent::create(SCREEN_WIDTH / 2, 130));
    registry.addComponent(subtitle, TextComponent::create("Multiplayer Lobby", 20,
        Color{200, 200, 220}));
    registry.addComponent(subtitle, LayerComponent::create(RenderLayer::UI));
    
    // Server IP input field
    auto ipLabel = registry.createEntity();
    registry.addComponent(ipLabel, TransformComponent::create(centerX - 100, startY));
    registry.addComponent(ipLabel, TextComponent::create("Server IP:", 16,
        Color{220, 220, 230}));
    registry.addComponent(ipLabel, LayerComponent::create(RenderLayer::UI));
    
    auto ipField = registry.createEntity();
    registry.addComponent(ipField, TransformComponent::create(centerX, startY));
    registry.addComponent(ipField, BoxComponent::create(280, 40,
        Color{30, 30, 40}, Color{70, 70, 90}));
    auto ipInput = InputFieldComponent::ipField("127.0.0.1");
    ipInput.placeholder = "Server IP address";
    registry.addComponent(ipField, ipInput);
    registry.addComponent(ipField, TextComponent::create("127.0.0.1", 16));
    registry.addComponent(ipField, FocusableComponent::create(0));
    registry.addComponent(ipField, LayerComponent::create(RenderLayer::UI));
    ipInputEntity_ = ipField;
    
    // Port input field
    auto portLabel = registry.createEntity();
    registry.addComponent(portLabel, TransformComponent::create(centerX - 100, startY + 60));
    registry.addComponent(portLabel, TextComponent::create("Port:", 16,
        Color{220, 220, 230}));
    registry.addComponent(portLabel, LayerComponent::create(RenderLayer::UI));
    
    auto portField = registry.createEntity();
    registry.addComponent(portField, TransformComponent::create(centerX, startY + 60));
    registry.addComponent(portField, BoxComponent::create(280, 40,
        Color{30, 30, 40}, Color{70, 70, 90}));
    auto portInput = InputFieldComponent::portField("50010");
    portInput.placeholder = "Port number";
    registry.addComponent(portField, portInput);
    registry.addComponent(portField, TextComponent::create("50010", 16));
    registry.addComponent(portField, FocusableComponent::create(1));
    registry.addComponent(portField, LayerComponent::create(RenderLayer::UI));
    portInputEntity_ = portField;
    
    // Connect button
    auto connectBtn = registry.createEntity();
    registry.addComponent(connectBtn, TransformComponent::create(centerX, startY + 130));
    registry.addComponent(connectBtn, BoxComponent::create(120, 45,
        Color{50, 100, 180}, Color{80, 140, 220}));
    registry.addComponent(connectBtn, ButtonComponent::create("Connect", [this]() {
        onConnectClicked();
    }));
    registry.addComponent(connectBtn, TextComponent::create("Connect", 18));
    registry.addComponent(connectBtn, FocusableComponent::create(2));
    registry.addComponent(connectBtn, LayerComponent::create(RenderLayer::UI));
    
    // Settings button
    auto settingsBtn = registry.createEntity();
    registry.addComponent(settingsBtn, TransformComponent::create(centerX + 140, startY + 130));
    registry.addComponent(settingsBtn, BoxComponent::create(120, 45,
        Color{50, 50, 70}, Color{80, 80, 110}));
    registry.addComponent(settingsBtn, ButtonComponent::create("Settings", [this]() {
        openSettings_ = true;
        done_ = true;
    }));
    registry.addComponent(settingsBtn, TextComponent::create("Settings", 18));
    registry.addComponent(settingsBtn, FocusableComponent::create(3));
    registry.addComponent(settingsBtn, LayerComponent::create(RenderLayer::UI));
    
    // Exit button
    auto exitBtn = registry.createEntity();
    registry.addComponent(exitBtn, TransformComponent::create(SCREEN_WIDTH / 2 - 60, 550));
    registry.addComponent(exitBtn, BoxComponent::create(120, 40,
        Color{70, 50, 50}, Color{120, 80, 80}));
    registry.addComponent(exitBtn, ButtonComponent::create("Exit", [this]() {
        exitRequested_ = true;
        done_ = true;
    }));
    registry.addComponent(exitBtn, TextComponent::create("Exit", 16));
    registry.addComponent(exitBtn, FocusableComponent::create(4));
    registry.addComponent(exitBtn, LayerComponent::create(RenderLayer::UI));
    
    // Error text (initially hidden)
    if (!initialError_.empty()) {
        setError(registry, initialError_);
    }
}
```

## Connection Flow

```cpp
void ConnectionMenu::onConnectClicked()
{
    // Get IP and port from input fields
    auto& ipInput = registry.getComponent<InputFieldComponent>(ipInputEntity_);
    auto& portInput = registry.getComponent<InputFieldComponent>(portInputEntity_);
    
    // Validate inputs
    if (ipInput.value.empty()) {
        setError(registry, "Please enter server IP");
        return;
    }
    
    if (portInput.value.empty()) {
        setError(registry, "Please enter port number");
        return;
    }
    
    // Show connecting status
    connecting_ = true;
    connectingStartTime_ = std::chrono::steady_clock::now();
    showConnectingText(registry);
    
    // Store connection info (actual connection happens in MenuRunner)
    connectionIP_ = ipInput.value;
    connectionPort_ = portInput.value;
}

void ConnectionMenu::showConnectingText(Registry& registry)
{
    connectingText_ = registry.createEntity();
    registry.addComponent(connectingText_, 
        TransformComponent::create(SCREEN_WIDTH / 2, 400));
    registry.addComponent(connectingText_, 
        TextComponent::create("Connecting to server.", 16, Color{200, 220, 255}));
    registry.addComponent(connectingText_, 
        LayerComponent::create(RenderLayer::UI));
}

void ConnectionMenu::updateConnectingText(Registry& registry)
{
    if (!connecting_ || connectingText_ == 0) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - connectingStartTime_).count();
    
    // Animate dots: . .. ... . .. ...
    if (elapsed % 500 < 250) {
        dotCount_ = (dotCount_ % 3) + 1;
        std::string dots(dotCount_, '.');
        
        auto& text = registry.getComponent<TextComponent>(connectingText_);
        text.content = "Connecting to server" + dots;
    }
}
```

## Error Display

```cpp
void ConnectionMenu::setError(Registry& registry, const std::string& message)
{
    // Remove old error if exists
    if (errorText_ != 0 && registry.entityExists(errorText_)) {
        registry.destroyEntity(errorText_);
    }
    
    // Create new error message
    errorText_ = registry.createEntity();
    registry.addComponent(errorText_, 
        TransformComponent::create(SCREEN_WIDTH / 2, 370));
    registry.addComponent(errorText_, 
        TextComponent::create("⚠ " + message, 14, Color{255, 100, 100}));
    registry.addComponent(errorText_, 
        LayerComponent::create(RenderLayer::UI));
}
```

## Result Handling

```cpp
Result ConnectionMenu::getResult(Registry& registry) const
{
    Result result;
    
    if (connecting_) {
        result.connected = true;
        
        auto& ipInput = registry.getComponent<InputFieldComponent>(ipInputEntity_);
        auto& portInput = registry.getComponent<InputFieldComponent>(portInputEntity_);
        
        result.ip = ipInput.value;
        result.port = portInput.value;
    }
    
    result.openSettings = openSettings_;
    result.exitRequested = exitRequested_;
    
    return result;
}
```

## Best Practices

1. **Default Values**: Pre-fill with localhost for quick testing
2. **Input Validation**: Validate IP format and port range
3. **Timeout Handling**: Show timeout errors clearly
4. **Animation**: Use animated dots for connection status
5. **Error Persistence**: Keep errors visible until next attempt

## Related Menus

- [LoginMenu](login-menu.md) - Next menu after successful connection
- [SettingsMenu](settings-menu.md) - Accessible from this menu

## Related Components

- [InputFieldComponent](../components/input-field-component.md) - IP/port input
- [ButtonComponent](../components/button-component.md) - Interactive buttons

## Related Systems

- [InputFieldSystem](../systems/input-field-system.md) - Text input handling
- [ButtonSystem](../systems/button-system.md) - Button interactions
