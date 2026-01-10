#include "ui/LoginMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/FocusableComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

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

LoginMenu::LoginMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn,
                     ThreadSafeQueue<NotificationData>& broadcastQueue)
    : fonts_(fonts), textures_(textures), lobbyConn_(lobbyConn), broadcastQueue_(broadcastQueue)
{}

void LoginMenu::create(Registry& registry)
{
    done_          = false;
    openRegister_  = false;
    exitRequested_ = false;
    authenticated_ = false;

    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    createBackground(registry, textures_);
    createLogo(registry, textures_);

    createLabel(registry, 440.0F, 285.0F, "Username");
    createLabel(registry, 440.0F, 385.0F, "Password");

    usernameInput_ = createInputField(registry, 440.0F, 320.0F, InputFieldComponent::create("", 32), 0);
    passwordInput_ = createInputField(registry, 440.0F, 420.0F, InputFieldComponent::password("", 64), 1);

    createButton(registry, 440.0F, 520.0F, "Login", Color(0, 120, 200), [this, &registry]() {
        Logger::instance().info("Login clicked");
        handleLoginAttempt(registry);
    });

    createButton(registry, 660.0F, 520.0F, "Register", Color(80, 150, 80), [this]() {
        Logger::instance().info("Register clicked");
        done_         = true;
        openRegister_ = true;
    });

    createButton(registry, 1050.0F, 560.0F, "Back", Color(100, 100, 100), [this]() {
        Logger::instance().info("Back clicked - returning to server selection");
        done_          = true;
        backRequested_ = true;
    });

    createButton(registry, 1050.0F, 620.0F, "Quit", Color(120, 50, 50), [this]() {
        Logger::instance().info("Quit clicked");
        done_          = true;
        exitRequested_ = true;
    });
}

void LoginMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool LoginMenu::isDone() const
{
    return done_;
}

void LoginMenu::handleEvent(Registry& /* registry */, const Event& /* event */) {}

void LoginMenu::render(Registry& /* registry */, Window& /* window */) {}

void LoginMenu::update(Registry& registry, float dt)
{
    lobbyConn_.poll(broadcastQueue_);

    if (isLoading_) {
        if (lobbyConn_.hasLoginResult()) {
            auto response = lobbyConn_.popLoginResult();
            isLoading_    = false;

            if (response.has_value() && response->success) {
                Logger::instance().info("Login successful for user: " + username_);
                authenticated_ = true;
                userId_        = response->userId;
                token_         = response->token;
                done_          = true;
            } else {
                std::string errorMsg = "Login failed";
                if (response.has_value()) {
                    switch (response->errorCode) {
                        case AuthErrorCode::InvalidCredentials:
                            errorMsg = "Invalid username or password";
                            break;
                        case AuthErrorCode::AlreadyConnected:
                            errorMsg = "Account already connected";
                            break;
                        case AuthErrorCode::ServerError:
                            errorMsg = "Server error occurred";
                            break;
                        default:
                            errorMsg = "Login failed";
                            break;
                    }
                } else {
                    errorMsg = "Login failed: No response";
                }
                setError(registry, errorMsg);
                Logger::instance().warn("Login failed: " + errorMsg);
            }
        }
        return;
    }

    heartbeatTimer_ += dt;
    if (heartbeatTimer_ >= 1.0F) {
        heartbeatTimer_ = 0.0F;
        if (!lobbyConn_.ping()) {
            Logger::instance().warn("[Heartbeat] Ping failed in Login Menu");
            consecutiveFailures_++;
        } else {
            consecutiveFailures_ = 0;
        }

        if (consecutiveFailures_ >= 2) {
            Logger::instance().error("[Heartbeat] Server lost in Login Menu");
            setError(registry, "Server connection lost");
            backRequested_ = true;
            done_          = true;
        }
    }
}

void LoginMenu::handleLoginAttempt(Registry& registry)
{
    std::string username;
    std::string password;

    if (usernameInput_ != 0 && registry.has<InputFieldComponent>(usernameInput_)) {
        username = registry.get<InputFieldComponent>(usernameInput_).value;
    }

    if (passwordInput_ != 0 && registry.has<InputFieldComponent>(passwordInput_)) {
        password = registry.get<InputFieldComponent>(passwordInput_).value;
    }

    if (username.empty() || password.empty()) {
        setError(registry, "Username and password are required");
        return;
    }

    if (isLoading_)
        return;

    username_ = username;
    lobbyConn_.sendLogin(username, password);
    isLoading_ = true;
}

LoginMenu::Result LoginMenu::getResult(Registry& registry) const
{
    (void) registry;
    Result res;
    res.authenticated = authenticated_;
    res.openRegister  = openRegister_;
    res.backRequested = backRequested_;
    res.exitRequested = exitRequested_;
    res.userId        = userId_;
    res.username      = username_;
    res.token         = token_;
    return res;
}

void LoginMenu::setError(Registry& registry, const std::string& message)
{
    (void) registry;
    broadcastQueue_.push(NotificationData{message, 3.0F});
}

void LoginMenu::reset()
{
    done_          = false;
    openRegister_  = false;
    backRequested_ = false;
    exitRequested_ = false;
    authenticated_ = false;
    isLoading_     = false;
    userId_        = 0;
    username_.clear();
    token_.clear();
    heartbeatTimer_      = 0.0F;
    consecutiveFailures_ = 0;
}
