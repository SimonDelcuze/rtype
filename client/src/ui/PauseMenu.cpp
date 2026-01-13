#include "ui/PauseMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

namespace
{
    EntityId createOverlay(Registry& registry)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 0.0F;
        transform.y     = 0.0F;

        auto box = BoxComponent::create(1280.0F, 720.0F, Color{0, 0, 0, 150}, Color{0, 0, 0, 150});
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<LayerComponent>(entity, LayerComponent::create(200));
        return entity;
    }

    EntityId createMenuBox(Registry& registry)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 440.0F;
        transform.y     = 220.0F;

        auto box = BoxComponent::create(400.0F, 280.0F, Color{30, 30, 50, 240}, Color{60, 60, 100, 255});
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<LayerComponent>(entity, LayerComponent::create(201));
        return entity;
    }

    EntityId createCenteredText(Registry& registry, float y, const std::string& content, unsigned int size, Color color,
                                int layer = 202)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 640.0F - (content.length() * size * 0.3F);
        transform.y     = y;

        auto text    = TextComponent::create("ui", size, color);
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        registry.emplace<LayerComponent>(entity, LayerComponent::create(layer));
        return entity;
    }

    EntityId createCenteredButton(Registry& registry, float y, const std::string& label, Color fill,
                                  std::function<void()> onClick, int layer = 202)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 490.0F;
        transform.y     = y;

        auto box       = BoxComponent::create(300.0F, 50.0F, fill,
                                              Color{static_cast<uint8_t>(fill.r + 40), static_cast<uint8_t>(fill.g + 40),
                                              static_cast<uint8_t>(fill.b + 40)});
        box.focusColor = Color{100, 200, 255};
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        registry.emplace<LayerComponent>(entity, LayerComponent::create(layer));
        return entity;
    }

} // namespace

PauseMenu::PauseMenu(FontManager& fonts) : fonts_(fonts) {}

void PauseMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundOverlay_ = createOverlay(registry);
    menuBox_           = createMenuBox(registry);
    titleText_         = createCenteredText(registry, 250.0F, "PAUSE", 48, Color::White, 202);

    resumeButton_ =
        createCenteredButton(registry, 330.0F, "Reprendre", Color{50, 120, 50}, [this]() { onResumeClicked(); }, 202);

    quitButton_ = createCenteredButton(
        registry, 400.0F, "Retour au menu", Color{120, 50, 50}, [this]() { onQuitClicked(); }, 202);

    Logger::instance().info("[PauseMenu] Created pause menu");
}

void PauseMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundOverlay_)) {
        registry.destroyEntity(backgroundOverlay_);
    }
    if (registry.isAlive(menuBox_)) {
        registry.destroyEntity(menuBox_);
    }
    if (registry.isAlive(titleText_)) {
        registry.destroyEntity(titleText_);
    }
    if (registry.isAlive(resumeButton_)) {
        registry.destroyEntity(resumeButton_);
    }
    if (registry.isAlive(quitButton_)) {
        registry.destroyEntity(quitButton_);
    }

    Logger::instance().info("[PauseMenu] Destroyed pause menu");
}

bool PauseMenu::isDone() const
{
    return done_;
}

void PauseMenu::handleEvent(Registry&, const Event& event)
{
    if (event.type == EventType::KeyPressed && event.key.code == KeyCode::Escape) {
        onResumeClicked();
    }
}

void PauseMenu::render(Registry& registry, Window& window)
{
    renderRectangle(registry, backgroundOverlay_, window);
    renderRectangle(registry, menuBox_, window);
    renderText(registry, titleText_, window);
    renderButton(registry, resumeButton_, window, 90.0F, 12.0F);
    renderButton(registry, quitButton_, window, 60.0F, 12.0F);
}

void PauseMenu::renderRectangle(Registry& registry, EntityId entityId, Window& window)
{
    if (registry.isAlive(entityId) && registry.has<BoxComponent>(entityId)) {
        (void) window;
    }
}

void PauseMenu::renderText(Registry& registry, EntityId entityId, Window& window)
{
    if (registry.isAlive(entityId) && registry.has<TextComponent>(entityId)) {
        const auto& text      = registry.get<TextComponent>(entityId);
        const auto& transform = registry.get<TransformComponent>(entityId);

        auto font = fonts_.get(text.fontId);
        if (font != nullptr) {
            GraphicsFactory factory;
            auto sfText = factory.createText();
            sfText->setFont(*font);
            sfText->setString(text.content);
            sfText->setCharacterSize(text.characterSize);
            sfText->setPosition(Vector2f{transform.x, transform.y});
            sfText->setFillColor(text.color);
            window.draw(*sfText);
        }
    }
}

void PauseMenu::renderButton(Registry& registry, EntityId entityId, Window& window, float labelOffsetX,
                             float labelOffsetY)
{
    if (registry.isAlive(entityId) && registry.has<ButtonComponent>(entityId)) {
        const auto& transform = registry.get<TransformComponent>(entityId);
        const auto& button    = registry.get<ButtonComponent>(entityId);

        auto font = fonts_.get("ui");
        if (font != nullptr) {
            GraphicsFactory factory;
            auto label = factory.createText();
            label->setFont(*font);
            label->setString(button.label);
            label->setCharacterSize(22);
            label->setPosition(Vector2f{transform.x + labelOffsetX, transform.y + labelOffsetY});
            label->setFillColor(Color::White);
            window.draw(*label);
        }
    }
}

void PauseMenu::onResumeClicked()
{
    Logger::instance().info("[PauseMenu] Resume clicked");
    result_ = Result::Resume;
    done_   = true;
}

void PauseMenu::onQuitClicked()
{
    Logger::instance().info("[PauseMenu] Quit clicked");
    result_ = Result::Quit;
    done_   = true;
}
