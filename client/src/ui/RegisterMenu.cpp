#include "ui/RegisterMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/FocusableComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

#include <cctype>
#include <utility>

namespace
{

    EntityId createBackground(Registry& registry, TextureManager& textures)
    {
        if (!textures.has("menu_bg"))
            textures.load("menu_bg", "client/assets/backgrounds/menu.jpg");

        auto tex = textures.get("menu_bg");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 0.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.25F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createLogo(Registry& registry, TextureManager& textures)
    {
        if (!textures.has("logo"))
            textures.load("logo", "client/assets/other/rtype-logo.png");

        auto tex = textures.get("logo");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 325.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.0F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createLabel(Registry& registry, float x, float y, const std::string& content, std::uint32_t size = 24)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto text    = TextComponent::create("ui", size, Color(200, 200, 200));
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createInputField(Registry& registry, float x, float y, InputFieldComponent field, int tabOrder)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box       = BoxComponent::create(400.0F, 50.0F, Color(50, 50, 50), Color(100, 100, 100));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<InputFieldComponent>(entity, field);
        registry.emplace<FocusableComponent>(entity, FocusableComponent::create(tabOrder));
        return entity;
    }

    EntityId createButton(Registry& registry, float x, float y, const std::string& label, Color fill,
                          std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box =
            BoxComponent::create(180.0F, 50.0F, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

} // namespace

RegisterMenu::RegisterMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn,
                           ThreadSafeQueue<NotificationData>& broadcastQueue)
    : fonts_(fonts), textures_(textures), lobbyConn_(lobbyConn), broadcastQueue_(broadcastQueue)
{}

void RegisterMenu::create(Registry& registry)
{
    done_          = false;
    backToLogin_   = false;
    exitRequested_ = false;
    registered_    = false;

    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    createBackground(registry, textures_);
    createLogo(registry, textures_);

    createLabel(registry, 440.0F, 250.0F, "Username");
    createLabel(registry, 440.0F, 350.0F, "Password");
    createLabel(registry, 440.0F, 450.0F, "Confirm Password");

    usernameInput_        = createInputField(registry, 440.0F, 285.0F, InputFieldComponent::create("", 32), 0);
    passwordInput_        = createInputField(registry, 440.0F, 385.0F, InputFieldComponent::password("", 64), 1);
    confirmPasswordInput_ = createInputField(registry, 440.0F, 485.0F, InputFieldComponent::password("", 64), 2);

    createButton(registry, 550.0F, 585.0F, "Register", Color(80, 150, 80), [this, &registry]() {
        Logger::instance().info("Register clicked");
        handleRegisterAttempt(registry);
    });

    createButton(registry, 1050.0F, 560.0F, "Back", Color(100, 100, 100), [this]() {
        Logger::instance().info("Back to Login clicked");
        done_        = true;
        backToLogin_ = true;
    });

    createButton(registry, 1050.0F, 620.0F, "Quit", Color(120, 50, 50), [this]() {
        Logger::instance().info("Quit clicked");
        done_          = true;
        exitRequested_ = true;
    });
}

void RegisterMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool RegisterMenu::isDone() const
{
    return done_;
}

void RegisterMenu::handleEvent(Registry& /* registry */, const Event& /* event */) {}

void RegisterMenu::render(Registry& /* registry */, Window& /* window */) {}

void RegisterMenu::update(Registry& registry, float dt)
{
    lobbyConn_.poll(broadcastQueue_);

    if (isLoading_) {
        if (lobbyConn_.hasRegisterResult()) {
            auto response = lobbyConn_.popRegisterResult();
            isLoading_    = false;

            if (response.has_value() && response->success) {
                Logger::instance().info("Registration successful for user: " + username_);
                registered_ = true;
                userId_     = response->userId;
                done_       = true;
            } else {
                std::string errorMsg = "Registration failed";
                if (response.has_value()) {
                    switch (response->errorCode) {
                        case AuthErrorCode::UsernameTaken:
                            errorMsg = "Username is already taken";
                            break;
                        case AuthErrorCode::WeakPassword:
                            errorMsg = "Password is too weak";
                            break;
                        case AuthErrorCode::ServerError:
                            errorMsg = "Server error occurred";
                            break;
                        default:
                            errorMsg = "Registration failed";
                            break;
                    }
                } else {
                    errorMsg = "Registration failed: No response";
                }
                setError(registry, errorMsg);
                Logger::instance().warn("Registration failed: " + errorMsg);
            }
        }
        return;
    }

    if (pingPending_ && lobbyConn_.hasRoomListResult()) {
        lobbyConn_.popRoomListResult();
        pingPending_         = false;
        consecutiveFailures_ = 0;
    }

    heartbeatTimer_ += dt;
    if (heartbeatTimer_ >= 2.0F) {
        heartbeatTimer_ = 0.0F;

        if (pingPending_) {
            Logger::instance().warn("[Heartbeat] Ping timeout in Register Menu");
            consecutiveFailures_++;
            pingPending_ = false;
        }

        if (consecutiveFailures_ >= 3) {
            Logger::instance().error("[Heartbeat] Server lost in Register Menu");
            setError(registry, "Server connection lost");
            backToLogin_ = true;
            done_        = true;
        } else {
            lobbyConn_.sendRequestRoomList();
            pingPending_ = true;
        }
    }
}

void RegisterMenu::handleRegisterAttempt(Registry& registry)
{
    std::string username;
    std::string password;
    std::string confirmPassword;

    if (usernameInput_ != 0 && registry.has<InputFieldComponent>(usernameInput_)) {
        username = registry.get<InputFieldComponent>(usernameInput_).value;
    }

    if (passwordInput_ != 0 && registry.has<InputFieldComponent>(passwordInput_)) {
        password = registry.get<InputFieldComponent>(passwordInput_).value;
    }

    if (confirmPasswordInput_ != 0 && registry.has<InputFieldComponent>(confirmPasswordInput_)) {
        confirmPassword = registry.get<InputFieldComponent>(confirmPasswordInput_).value;
    }

    if (username.empty() || password.empty() || confirmPassword.empty()) {
        setError(registry, "All fields are required");
        return;
    }

    if (username.length() < 3 || username.length() > 32) {
        setError(registry, "Username must be 3-32 characters");
        return;
    }

    for (char c : username) {
        if (!std::isalnum(c) && c != '_') {
            setError(registry, "Username can only contain letters, numbers, and underscores");
            return;
        }
    }

    std::string passwordError;
    if (!validatePassword(password, passwordError)) {
        setError(registry, passwordError);
        return;
    }

    if (password != confirmPassword) {
        setError(registry, "Passwords do not match");
        return;
    }

    if (isLoading_)
        return;

    Logger::instance().info("Attempting registration (async) for user: " + username);
    username_ = username;

    lobbyConn_.sendRegister(username, password);
    isLoading_ = true;
}

RegisterMenu::Result RegisterMenu::getResult(Registry& registry) const
{
    (void) registry;
    Result res;
    res.registered    = registered_;
    res.backToLogin   = backToLogin_;
    res.exitRequested = exitRequested_;
    res.userId        = userId_;
    res.username      = username_;
    return res;
}

void RegisterMenu::setError(Registry& registry, const std::string& message)
{
    (void) registry;
    broadcastQueue_.push(NotificationData{message, 3.0F});
}

void RegisterMenu::reset()
{
    done_          = false;
    backToLogin_   = false;
    exitRequested_ = false;
    registered_    = false;
    isLoading_     = false;
    userId_        = 0;
    username_.clear();
    heartbeatTimer_      = 0.0F;
    consecutiveFailures_ = 0;
    pingPending_         = false;
}

bool RegisterMenu::validatePassword(const std::string& password, std::string& errorMsg)
{
    if (password.length() < 8) {
        errorMsg = "Password must be at least 8 characters";
        return false;
    }
    return true;
}
