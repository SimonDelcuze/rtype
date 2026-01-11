# LoginMenu

## Overview

The `LoginMenu` handles user authentication for the R-Type multiplayer lobby system. It provides username and password input fields, validates credentials with the lobby server, and transitions to the lobby browser upon successful authentication.

## Class Structure

```cpp
class LoginMenu : public IMenu
{
  public:
    struct Result
    {
        bool authenticated = false;
        bool openRegister  = false;
        bool backRequested = false;
        bool exitRequested = false;
        std::uint32_t userId{0};
        std::string username;
        std::string token;
    };

    LoginMenu(FontManager& fonts, TextureManager& textures, 
              LobbyConnection& lobbyConn,
              ThreadSafeQueue<NotificationData>& broadcastQueue);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const;
    void setError(Registry& registry, const std::string& message);
    void reset();

  private:
    void handleLoginAttempt(Registry& registry);

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection& lobbyConn_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;

    bool done_          = false;
    bool openRegister_  = false;
    bool backRequested_ = false;
    bool exitRequested_ = false;
    bool authenticated_ = false;
    bool isLoading_     = false;

    std::uint32_t userId_{0};
    std::string username_;
    std::string token_;

    float heartbeatTimer_{0.0F};
    int consecutiveFailures_{0};

    EntityId usernameInput_ = 0;
    EntityId passwordInput_ = 0;
};
```

## UI Layout

```
┌────────────────────────────────────────────────┐
│                                                │
│                  R - T Y P E                   │
│                    Login                       │
│                                                │
│   ┌──────────────────────────────────────┐    │
│   │ Username: [________________]         │    │
│   └──────────────────────────────────────┘    │
│                                                │
│   ┌──────────────────────────────────────┐    │
│   │ Password: [****************]         │    │
│   └──────────────────────────────────────┘    │
│                                                │
│        [ Login ]      [ Register ]            │
│                                                │
│   ✕ Invalid credentials                       │
│   (if error)                                   │
│                                                │
│   ⏳ Logging in...                             │
│   (if loading)                                 │
│                                                │
│   [ Back ]                  [ Exit ]          │
└────────────────────────────────────────────────┘
```

## Menu Creation

```cpp
void LoginMenu::create(Registry& registry)
{
    const float centerX = SCREEN_WIDTH / 2 - 140;
    const float startY = 180;
    
    // Title
    auto title = registry.createEntity();
    registry.addComponent(title, TransformComponent::create(SCREEN_WIDTH / 2, 80));
    registry.addComponent(title, TextComponent::create("R-TYPE", 48));
    registry.addComponent(title, LayerComponent::create(RenderLayer::UI));
    
    auto subtitle = registry.createEntity();
    registry.addComponent(subtitle, TransformComponent::create(SCREEN_WIDTH / 2, 130));
    registry.addComponent(subtitle, TextComponent::create("Login", 24));
    registry.addComponent(subtitle, LayerComponent::create(RenderLayer::UI));
    
    // Username field
    auto usernameLabel = registry.createEntity();
    registry.addComponent(usernameLabel, TransformComponent::create(centerX - 80, startY));
    registry.addComponent(usernameLabel, TextComponent::create("Username:", 16));
    registry.addComponent(usernameLabel, LayerComponent::create(RenderLayer::UI));
    
    usernameInput_ = registry.createEntity();
    registry.addComponent(usernameInput_, TransformComponent::create(centerX, startY));
    registry.addComponent(usernameInput_, BoxComponent::create(280, 40,
        Color{30, 30, 40}, Color{70, 70, 90}));
    auto usernameField = InputFieldComponent::create("", 24);
    usernameField.placeholder = "Enter username";
    registry.addComponent(usernameInput_, usernameField);
    registry.addComponent(usernameInput_, TextComponent::create("", 16));
    registry.addComponent(usernameInput_, FocusableComponent::create(0));
    registry.addComponent(usernameInput_, LayerComponent::create(RenderLayer::UI));
    
    // Password field
    auto passwordLabel = registry.createEntity();
    registry.addComponent(passwordLabel, TransformComponent::create(centerX - 80, startY + 60));
    registry.addComponent(passwordLabel, TextComponent::create("Password:", 16));
    registry.addComponent(passwordLabel, LayerComponent::create(RenderLayer::UI));
    
    passwordInput_ = registry.createEntity();
    registry.addComponent(passwordInput_, TransformComponent::create(centerX, startY + 60));
    registry.addComponent(passwordInput_, BoxComponent::create(280, 40,
        Color{30, 30, 40}, Color{70, 70, 90}));
    auto passwordField = InputFieldComponent::password("", 32);
    passwordField.placeholder = "Enter password";
    registry.addComponent(passwordInput_, passwordField);
    registry.addComponent(passwordInput_, TextComponent::create("", 16));
    registry.addComponent(passwordInput_, FocusableComponent::create(1));
    registry.addComponent(passwordInput_, LayerComponent::create(RenderLayer::UI));
    
    // Login button
    auto loginBtn = registry.createEntity();
    registry.addComponent(loginBtn, TransformComponent::create(centerX, startY + 130));
    registry.addComponent(loginBtn, BoxComponent::create(120, 45,
        Color{50, 100, 180}, Color{80, 140, 220}));
    registry.addComponent(loginBtn, ButtonComponent::create("Login", [this, &registry]() {
        handleLoginAttempt(registry);
    }));
    registry.addComponent(loginBtn, TextComponent::create("Login", 18));
    registry.addComponent(loginBtn, FocusableComponent::create(2));
    registry.addComponent(loginBtn, LayerComponent::create(RenderLayer::UI));
    
    // Register button
    auto registerBtn = registry.createEntity();
    registry.addComponent(registerBtn, TransformComponent::create(centerX + 140, startY + 130));
    registry.addComponent(registerBtn, BoxComponent::create(120, 45,
        Color{50, 120, 80}, Color{80, 160, 120}));
    registry.addComponent(registerBtn, ButtonComponent::create("Register", [this]() {
        openRegister_ = true;
        done_ = true;
    }));
    registry.addComponent(registerBtn, TextComponent::create("Register", 18));
    registry.addComponent(registerBtn, FocusableComponent::create(3));
    registry.addComponent(registerBtn, LayerComponent::create(RenderLayer::UI));
    
    // Back button
    auto backBtn = registry.createEntity();
    registry.addComponent(backBtn, TransformComponent::create(100, 550));
    registry.addComponent(backBtn, BoxComponent::create(100, 40,
        Color{50, 50, 60}, Color{80, 80, 100}));
    registry.addComponent(backBtn, ButtonComponent::create("Back", [this]() {
        backRequested_ = true;
        done_ = true;
    }));
    registry.addComponent(backBtn, TextComponent::create("Back", 16));
    registry.addComponent(backBtn, FocusableComponent::create(4));
    registry.addComponent(backBtn, LayerComponent::create(RenderLayer::UI));
    
    // Exit button
    auto exitBtn = registry.createEntity();
    registry.addComponent(exitBtn, TransformComponent::create(SCREEN_WIDTH - 220, 550));
    registry.addComponent(exitBtn, BoxComponent::create(100, 40,
        Color{70, 50, 50}, Color{120, 80, 80}));
    registry.addComponent(exitBtn, ButtonComponent::create("Exit", [this]() {
        exitRequested_ = true;
        done_ = true;
    }));
    registry.addComponent(exitBtn, TextComponent::create("Exit", 16));
    registry.addComponent(exitBtn, FocusableComponent::create(5));
    registry.addComponent(exitBtn, LayerComponent::create(RenderLayer::UI));
}
```

## Login Flow

```cpp
void LoginMenu::handleLoginAttempt(Registry& registry)
{
    // Get credentials
    auto& usernameField = registry.getComponent<InputFieldComponent>(usernameInput_);
    auto& passwordField = registry.getComponent<InputFieldComponent>(passwordInput_);
    
    // Validate input
    if (usernameField.value.empty()) {
        broadcastQueue_.enqueue(NotificationData{
            "Please enter username",
            NotificationType::Error,
            3.0F
        });
        return;
    }
    
    if (passwordField.value.empty()) {
        broadcastQueue_.enqueue(NotificationData{
            "Please enter password",
            NotificationType::Error,
            3.0F
        });
        return;
    }
    
    // Send login request
    isLoading_ = true;
    
    LoginRequest request;
    request.username = usernameField.value;
    request.password = passwordField.value;
    
    if (!lobbyConn_.sendLoginRequest(request)) {
        isLoading_ = false;
        broadcastQueue_.enqueue(NotificationData{
            "Failed to send login request",
            NotificationType::Error,
            3.0F
        });
        return;
    }
}
```

## Update Loop

```cpp
void LoginMenu::update(Registry& registry, float dt)
{
    if (!isLoading_) return;
    
    // Check for login response
    LoginResponse response;
    if (lobbyConn_.tryGetLoginResponse(response)) {
        isLoading_ = false;
        
        if (response.success) {
            // Authentication successful
            authenticated_ = true;
            userId_ = response.userId;
            username_ = response.username;
            token_ = response.token;
            done_ = true;
            
            broadcastQueue_.enqueue(NotificationData{
                "Login successful!",
                NotificationType::Success,
                2.0F
            });
        } else {
            // Authentication failed
            consecutiveFailures_++;
            
            broadcastQueue_.enqueue(NotificationData{
                "Login failed: " + response.errorMessage,
                NotificationType::Error,
                4.0F
            });
            
            // Clear password field
            auto& passwordField = registry.getComponent<InputFieldComponent>(passwordInput_);
            passwordField.value.clear();
        }
    }
    
    // Heartbeat to keep connection alive
    heartbeatTimer_ += dt;
    if (heartbeatTimer_ >= 5.0F) {
        lobbyConn_.sendHeartbeat();
        heartbeatTimer_ = 0.0F;
    }
}
```

## Result Retrieval

```cpp
LoginMenu::Result LoginMenu::getResult(Registry& registry) const
{
    Result result;
    result.authenticated = authenticated_;
    result.openRegister = openRegister_;
    result.backRequested = backRequested_;
    result.exitRequested = exitRequested_;
    result.userId = userId_;
    result.username = username_;
    result.token = token_;
    return result;
}
```

## Security Considerations

- Password field uses `InputFieldComponent::password()` for masked input
- Passwords are never logged or displayed
- Token received from server is stored for subsequent requests
- Failed attempts are tracked to detect brute force

## Network Integration

The menu communicates with the lobby server via `LobbyConnection`:

```cpp
// LobbyConnection interface
bool sendLoginRequest(const LoginRequest& request);
bool tryGetLoginResponse(LoginResponse& outResponse);
void sendHeartbeat();
```

## Related Menus

- [ConnectionMenu](connection-menu.md) - Previous menu
- [RegisterMenu](register-menu.md) - Account creation
- [LobbyMenu](lobby-menu.md) - Next menu after login

## Related Components

- [InputFieldComponent](../components/input-field-component.md)
- [ButtonComponent](../components/button-component.md)
- [FocusableComponent](../components/focusable-component.md)

## Related Systems

- [InputFieldSystem](../systems/input-field-system.md)
- [ButtonSystem](../systems/button-system.md)
- [NotificationSystem](../systems/notification-system.md)
