#include "ui/CreateRoomMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

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
                        Color color)
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

    EntityId createButton(Registry& registry, float x, float y, float width, float height, const std::string& label,
                          Color fill, std::function<void()> onClick)
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

    EntityId createInputField(Registry& registry, float x, float y, float width, const std::string& defaultValue)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box       = BoxComponent::create(width, 40.0F, Color(40, 40, 50), Color(60, 60, 70));
        box.focusColor = Color(80, 120, 200);
        registry.emplace<BoxComponent>(entity, box);

        auto input = InputFieldComponent::create(defaultValue, 32);
        registry.emplace<InputFieldComponent>(entity, input);
        return entity;
    }

} // namespace

CreateRoomMenu::CreateRoomMenu(FontManager& fonts, TextureManager& textures) : fonts_(fonts), textures_(textures) {}

void CreateRoomMenu::create(Registry& registry)
{
    done_            = false;
    result_          = Result{};
    passwordEnabled_ = false;

    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundEntity_ = createBackground(registry, textures_);
    logoEntity_       = createLogo(registry, textures_);

    titleEntity_ = createText(registry, 450.0F, 200.0F, "Create Room", 36, Color::White);

    roomNameLabelEntity_ = createText(registry, 400.0F, 270.0F, "Room Name:", 20, Color(200, 200, 200));
    roomNameInputEntity_ = createInputField(registry, 400.0F, 300.0F, 400.0F, "My Room");

    passwordLabelEntity_ = createText(registry, 400.0F, 360.0F, "Password (optional):", 20, Color(200, 200, 200));
    passwordInputEntity_ = createInputField(registry, 400.0F, 390.0F, 300.0F, "");

    passwordToggleEntity_ = createButton(registry, 720.0F, 390.0F, 120.0F, 40.0F, "Disabled", Color(80, 80, 80),
                                         [this]() { onTogglePassword(); });

    createButtonEntity_ = createButton(registry, 450.0F, 480.0F, 180.0F, 50.0F, "Create Room", Color(0, 150, 80),
                                       [this]() { onCreateClicked(); });

    cancelButtonEntity_ = createButton(registry, 650.0F, 480.0F, 150.0F, 50.0F, "Cancel", Color(120, 50, 50),
                                       [this]() { onCancelClicked(); });
}

void CreateRoomMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundEntity_))
        registry.destroyEntity(backgroundEntity_);
    if (registry.isAlive(logoEntity_))
        registry.destroyEntity(logoEntity_);
    if (registry.isAlive(titleEntity_))
        registry.destroyEntity(titleEntity_);
    if (registry.isAlive(roomNameLabelEntity_))
        registry.destroyEntity(roomNameLabelEntity_);
    if (registry.isAlive(roomNameInputEntity_))
        registry.destroyEntity(roomNameInputEntity_);
    if (registry.isAlive(passwordLabelEntity_))
        registry.destroyEntity(passwordLabelEntity_);
    if (registry.isAlive(passwordInputEntity_))
        registry.destroyEntity(passwordInputEntity_);
    if (registry.isAlive(passwordToggleEntity_))
        registry.destroyEntity(passwordToggleEntity_);
    if (registry.isAlive(createButtonEntity_))
        registry.destroyEntity(createButtonEntity_);
    if (registry.isAlive(cancelButtonEntity_))
        registry.destroyEntity(cancelButtonEntity_);
}

bool CreateRoomMenu::isDone() const
{
    return done_;
}

void CreateRoomMenu::handleEvent(Registry&, const Event&) {}

void CreateRoomMenu::render(Registry& registry, Window&)
{
    if (roomNameInputEntity_ != 0 && registry.has<InputFieldComponent>(roomNameInputEntity_)) {
        result_.roomName = registry.get<InputFieldComponent>(roomNameInputEntity_).value;
    }

    if (passwordEnabled_ && passwordInputEntity_ != 0 && registry.has<InputFieldComponent>(passwordInputEntity_)) {
        result_.password = registry.get<InputFieldComponent>(passwordInputEntity_).value;
    } else {
        result_.password.clear();
    }

    if (passwordToggleEntity_ != 0 && registry.has<ButtonComponent>(passwordToggleEntity_)) {
        registry.get<ButtonComponent>(passwordToggleEntity_).label = passwordEnabled_ ? "Enabled" : "Disabled";
    }
}

void CreateRoomMenu::onCreateClicked()
{
    Logger::instance().info("[CreateRoomMenu] Create clicked");
    result_.created = true;
    done_           = true;
}

void CreateRoomMenu::onCancelClicked()
{
    Logger::instance().info("[CreateRoomMenu] Cancel clicked");
    result_.cancelled = true;
    done_             = true;
}

void CreateRoomMenu::onTogglePassword()
{
    passwordEnabled_ = !passwordEnabled_;
    Logger::instance().info("[CreateRoomMenu] Password " + std::string(passwordEnabled_ ? "enabled" : "disabled"));
}
