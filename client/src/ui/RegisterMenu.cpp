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

RegisterMenu::RegisterMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn)
    : fonts_(fonts), textures_(textures), lobbyConn_(lobbyConn)
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

    createLabel(registry, 480.0F, 180.0F, "REGISTER", 36);
    createLabel(registry, 440.0F, 260.0F, "Username (3-32 chars):");
    createLabel(registry, 440.0F, 340.0F, "Password (8+ chars):");
    createLabel(registry, 440.0F, 420.0F, "Confirm Password:");

    usernameInput_        = createInputField(registry, 440.0F, 290.0F, InputFieldComponent::create("", 32), 0);
    passwordInput_        = createInputField(registry, 440.0F, 370.0F, InputFieldComponent::password("", 64), 1);
    confirmPasswordInput_ = createInputField(registry, 440.0F, 450.0F, InputFieldComponent::password("", 64), 2);

    createButton(registry, 440.0F, 540.0F, "Register", Color(80, 150, 80), [this, &registry]() {
        Logger::instance().info("Register clicked");
        handleRegisterAttempt(registry);
    });

    createButton(registry, 660.0F, 540.0F, "Back to Login", Color(80, 80, 80), [this]() {
        Logger::instance().info("Back to Login clicked");
        done_        = true;
        backToLogin_ = true;
    });

    createButton(registry, 1050.0F, 620.0F, "Quit", Color(120, 50, 50), [this]() {
        Logger::instance().info("Quit clicked");
        done_          = true;
        exitRequested_ = true;
    });

    errorText_ = createLabel(registry, 440.0F, 620.0F, "", 16);
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

RegisterMenu::Result RegisterMenu::getResult(Registry& /* registry */) const
{
    Result result;
    result.registered    = registered_;
    result.backToLogin   = backToLogin_;
    result.exitRequested = exitRequested_;
    result.userId        = userId_;
    result.username      = username_;
    return result;
}

void RegisterMenu::setError(Registry& registry, const std::string& message)
{
    if (errorText_ != 0 && registry.has<TextComponent>(errorText_)) {
        auto& text   = registry.get<TextComponent>(errorText_);
        text.content = message;
        text.color   = Color(255, 100, 100);
    }
}

void RegisterMenu::reset()
{
    done_          = false;
    backToLogin_   = false;
    exitRequested_ = false;
    registered_    = false;
    userId_        = 0;
    username_.clear();
}

bool RegisterMenu::validatePassword(const std::string& password, std::string& errorMsg)
{
    if (password.length() < 8) {
        errorMsg = "Password must be at least 8 characters";
        return false;
    }

    if (password.length() > 64) {
        errorMsg = "Password must be at most 64 characters";
        return false;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;

    for (char c : password) {
        if (std::isupper(c))
            hasUpper = true;
        if (std::islower(c))
            hasLower = true;
        if (std::isdigit(c))
            hasDigit = true;
    }

    if (!hasUpper || !hasLower || !hasDigit) {
        errorMsg = "Password must contain uppercase, lowercase, and digit";
        return false;
    }

    return true;
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

    Logger::instance().info("Attempting registration for user: " + username);

    auto response = lobbyConn_.registerUser(username, password);

    if (!response.has_value()) {
        setError(registry, "Connection error: Unable to reach server");
        Logger::instance().error("Registration failed: no response from server");
        return;
    }

    if (!response->success) {
        std::string errorMsg;
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
        setError(registry, errorMsg);
        Logger::instance().warn("Registration failed for user " + username + ": " + errorMsg);
        return;
    }

    Logger::instance().info("Registration successful for user: " + username);
    registered_ = true;
    userId_     = response->userId;
    username_   = username;
    done_       = true;
}
