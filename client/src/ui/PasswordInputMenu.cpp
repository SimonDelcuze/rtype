#include "ui/PasswordInputMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"

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

    EntityId createText(Registry& registry, float x, float y, const std::string& content, unsigned int size,
                        const Color& color)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto text    = TextComponent::create("ui", size, color);
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }

    EntityId createInputField(Registry& registry, float x, float y, float width, float height,
                              const std::string& defaultValue)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box = BoxComponent::create(width, height, Color(40, 40, 40), Color(60, 60, 60));
        registry.emplace<BoxComponent>(entity, box);

        auto input = InputFieldComponent::create(defaultValue, 32);
        registry.emplace<InputFieldComponent>(entity, input);

        return entity;
    }

    EntityId createButton(Registry& registry, float x, float y, float width, float height, const std::string& label,
                          const Color& fill, std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box =
            BoxComponent::create(width, height, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }
} // namespace

PasswordInputMenu::PasswordInputMenu(FontManager& fonts, TextureManager& textures)
    : fonts_(fonts), textures_(textures)
{}

void PasswordInputMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundEntity_ = createBackground(registry, textures_);
    logoEntity_       = createLogo(registry, textures_);

    titleEntity_ = createText(registry, 400.0F, 200.0F, "Enter Room Password", 32, Color::White);

    passwordLabelEntity_ = createText(registry, 400.0F, 300.0F, "Password:", 20, Color(200, 200, 200));

    passwordInputEntity_ = createInputField(registry, 400.0F, 340.0F, 400.0F, 50.0F, "");

    submitButtonEntity_ = createButton(registry, 400.0F, 420.0F, 180.0F, 50.0F, "Join Room", Color(0, 120, 200),
                                       [this]() { onSubmit(); });

    cancelButtonEntity_ = createButton(registry, 600.0F, 420.0F, 150.0F, 50.0F, "Cancel", Color(120, 50, 50),
                                       [this]() { onCancel(); });
}

void PasswordInputMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundEntity_))
        registry.destroyEntity(backgroundEntity_);
    if (registry.isAlive(logoEntity_))
        registry.destroyEntity(logoEntity_);
    if (registry.isAlive(titleEntity_))
        registry.destroyEntity(titleEntity_);
    if (registry.isAlive(passwordLabelEntity_))
        registry.destroyEntity(passwordLabelEntity_);
    if (registry.isAlive(passwordInputEntity_))
        registry.destroyEntity(passwordInputEntity_);
    if (registry.isAlive(submitButtonEntity_))
        registry.destroyEntity(submitButtonEntity_);
    if (registry.isAlive(cancelButtonEntity_))
        registry.destroyEntity(cancelButtonEntity_);
}

bool PasswordInputMenu::isDone() const
{
    return done_;
}

void PasswordInputMenu::handleEvent(Registry& registry, const Event& event)
{
    (void) registry;
    (void) event;
}

void PasswordInputMenu::render(Registry& registry, Window& window)
{
    (void) window;

    if (registry.has<InputFieldComponent>(passwordInputEntity_)) {
        result_.password = registry.get<InputFieldComponent>(passwordInputEntity_).value;
    }
}

void PasswordInputMenu::onSubmit()
{
    Logger::instance().info("[PasswordInputMenu] Submit clicked");
    result_.submitted = true;
    result_.cancelled = false;
    done_             = true;
}

void PasswordInputMenu::onCancel()
{
    Logger::instance().info("[PasswordInputMenu] Cancel clicked");
    result_.submitted = false;
    result_.cancelled = true;
    done_             = true;
}
