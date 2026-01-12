#include "ui/CreateRoomMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/InputFieldComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

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

    std::string toString(float value)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(0) << value;
        return ss.str();
    }

    std::string toStringLives(std::uint8_t lives)
    {
        return std::to_string(lives);
    }

    struct DifficultyPreset
    {
        float enemyMultiplier      = 1.0F;
        float playerSpeedMultiplier= 1.0F;
        float scoreMultiplier      = 1.0F;
        std::uint8_t lives         = 3;
    };

    DifficultyPreset presetFromMode(RoomDifficulty difficulty)
    {
        DifficultyPreset p{};
        switch (difficulty) {
            case RoomDifficulty::Noob:
                p.enemyMultiplier       = 1.0F;
                p.playerSpeedMultiplier = 1.0F;
                p.scoreMultiplier       = 1.0F;
                p.lives                 = 3;
                break;
            case RoomDifficulty::Hell:
                p.enemyMultiplier       = 1.33F;
                p.playerSpeedMultiplier = 0.67F;
                p.scoreMultiplier       = 1.5F;
                p.lives                 = 2;
                break;
            case RoomDifficulty::Nightmare:
                p.enemyMultiplier       = 1.66F;
                p.playerSpeedMultiplier = 0.34F;
                p.scoreMultiplier       = 2.0F;
                p.lives                 = 1;
                break;
            case RoomDifficulty::Custom:
            default:
                break;
        }
        return p;
    }

} // namespace

CreateRoomMenu::CreateRoomMenu(FontManager& fonts, TextureManager& textures) : fonts_(fonts), textures_(textures) {}

void CreateRoomMenu::create(Registry& registry)
{
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

CreateRoomMenu::ConfigRow CreateRoomMenu::createConfigRow(Registry& registry, float x, float y, const std::string& label,
                                                          const std::string& value)
{
    ConfigRow row;
    row.label  = createText(registry, x, y, label, 18, Color(200, 200, 200));
    row.input  = createInputField(registry, x + 220.0F, y - 5.0F, 120.0F, value);
    row.suffix = createText(registry, x + 350.0F, y, "", 16, Color(160, 160, 160));
    return row;
}

void CreateRoomMenu::destroyConfigRow(Registry& registry, const ConfigRow& row)
{
    if (registry.isAlive(row.label))
        registry.destroyEntity(row.label);
    if (registry.isAlive(row.input))
        registry.destroyEntity(row.input);
    if (registry.isAlive(row.suffix))
        registry.destroyEntity(row.suffix);
}

void CreateRoomMenu::setInputValue(Registry& registry, EntityId inputId, const std::string& value)
{
    if (!registry.has<InputFieldComponent>(inputId))
        return;
    auto& input = registry.get<InputFieldComponent>(inputId);
    if (input.value != value) {
        input.value = value;
    }
}

float CreateRoomMenu::readPercentField(Registry& registry, EntityId inputId, float minVal, float maxVal,
                                       float fallback)
{
    if (!registry.has<InputFieldComponent>(inputId))
        return fallback;
    const auto& comp = registry.get<InputFieldComponent>(inputId);
    try {
        float v = std::stof(comp.value);
        v       = std::clamp(v, minVal, maxVal);
        return v;
    } catch (...) {
        return fallback;
    }
}

std::uint8_t CreateRoomMenu::readLivesField(Registry& registry, EntityId inputId, std::uint8_t fallback)
{
    if (!registry.has<InputFieldComponent>(inputId))
        return fallback;
    const auto& comp = registry.get<InputFieldComponent>(inputId);
    try {
        int v = std::stoi(comp.value);
        v     = std::clamp(v, 1, 6);
        return static_cast<std::uint8_t>(v);
    } catch (...) {
        return fallback;
    }
}

void CreateRoomMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundEntity_))
        registry.destroyEntity(backgroundEntity_);
    if (registry.isAlive(logoEntity_))
        registry.destroyEntity(logoEntity_);
    if (registry.isAlive(titleEntity_))
        registry.destroyEntity(titleEntity_);
    if (registry.isAlive(difficultyTitleEntity_))
        registry.destroyEntity(difficultyTitleEntity_);
    if (registry.isAlive(configTitleEntity_))
        registry.destroyEntity(configTitleEntity_);
    for (auto id : difficultyButtons_) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
    destroyConfigRow(registry, enemyRow_);
    destroyConfigRow(registry, playerRow_);
    destroyConfigRow(registry, scoreRow_);
    destroyConfigRow(registry, livesRow_);
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

    updateDifficultyUI(registry);
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

void CreateRoomMenu::setDifficulty(RoomDifficulty difficulty)
{
    result_.difficulty = difficulty;
    if (difficulty == RoomDifficulty::Custom) {
        return;
    }
    DifficultyPreset cfg        = presetFromMode(difficulty);
    result_.enemyMultiplier     = cfg.enemyMultiplier;
    result_.playerSpeedMultiplier= cfg.playerSpeedMultiplier;
    result_.scoreMultiplier     = cfg.scoreMultiplier;
    result_.playerLives         = cfg.lives;
}

void CreateRoomMenu::updateDifficultyUI(Registry& registry)
{
    for (std::size_t i = 0; i < difficultyButtons_.size(); ++i) {
        if (!registry.has<BoxComponent>(difficultyButtons_[i]))
            continue;
        auto& box   = registry.get<BoxComponent>(difficultyButtons_[i]);
        bool active = (static_cast<int>(result_.difficulty) == static_cast<int>(i));
        box.fillColor    = active ? Color(0, 150, 80) : Color(50, 70, 90);
        box.outlineColor = active ? Color(0, 180, 110) : Color(80, 90, 110);
    }

    const bool isCustom = result_.difficulty == RoomDifficulty::Custom;
    if (!isCustom) {
        DifficultyPreset cfg            = presetFromMode(result_.difficulty);
        result_.enemyMultiplier         = cfg.enemyMultiplier;
        result_.playerSpeedMultiplier   = cfg.playerSpeedMultiplier;
        result_.scoreMultiplier         = cfg.scoreMultiplier;
        result_.playerLives             = cfg.lives;
    } else {
        float enemyPct =
            readPercentField(registry, enemyRow_.input, 50.0F, 200.0F, result_.enemyMultiplier * 100.0F);
        float playerPct =
            readPercentField(registry, playerRow_.input, 50.0F, 200.0F, result_.playerSpeedMultiplier * 100.0F);
        float scorePct =
            readPercentField(registry, scoreRow_.input, 50.0F, 200.0F, result_.scoreMultiplier * 100.0F);
        std::uint8_t lives = readLivesField(registry, livesRow_.input,
                                            result_.playerLives == 0 ? static_cast<std::uint8_t>(3)
                                                                     : result_.playerLives);

        result_.enemyMultiplier       = enemyPct / 100.0F;
        result_.playerSpeedMultiplier = playerPct / 100.0F;
        result_.scoreMultiplier       = scorePct / 100.0F;
        result_.playerLives           = lives;
    }

    setInputValue(registry, enemyRow_.input, toString(result_.enemyMultiplier * 100.0F));
    setInputValue(registry, playerRow_.input, toString(result_.playerSpeedMultiplier * 100.0F));
    setInputValue(registry, scoreRow_.input, toString(result_.scoreMultiplier * 100.0F));
    setInputValue(registry, livesRow_.input, toStringLives(result_.playerLives));

    auto setRowState = [&](const ConfigRow& row, bool locked) {
        if (registry.has<BoxComponent>(row.input)) {
            auto& box = registry.get<BoxComponent>(row.input);
            box.fillColor =
                locked ? Color(50, 50, 50, 160) : Color(40, 40, 50);
            box.outlineColor = locked ? Color(80, 80, 80) : Color(60, 60, 70);
        }
        if (registry.has<TextComponent>(row.suffix)) {
            registry.get<TextComponent>(row.suffix).content = locked ? "(locked)" : "(editable)";
        }
    };

    setRowState(enemyRow_, !isCustom);
    setRowState(playerRow_, !isCustom);
    setRowState(scoreRow_, !isCustom);
    setRowState(livesRow_, !isCustom);
}
