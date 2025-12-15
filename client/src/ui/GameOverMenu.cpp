#include "ui/GameOverMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

namespace
{
    EntityId createSemiTransparentBackground(Registry& registry)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 0.0F;
        transform.y     = 0.0F;

        auto box = BoxComponent::create(1280.0F, 720.0F, sf::Color(0, 0, 0, 180), sf::Color(0, 0, 0, 180));
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<LayerComponent>(entity, LayerComponent::create(100));
        return entity;
    }

    EntityId createCenteredText(Registry& registry, float y, const std::string& content, unsigned int size,
                                sf::Color color, int layer = 101)
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

    EntityId createCenteredButton(Registry& registry, float y, const std::string& label, sf::Color fill,
                                  std::function<void()> onClick, int layer = 101)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 540.0F;
        transform.y     = y;

        auto box       = BoxComponent::create(200.0F, 60.0F, fill, sf::Color(fill.r + 40, fill.g + 40, fill.b + 40));
        box.focusColor = sf::Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        registry.emplace<LayerComponent>(entity, LayerComponent::create(layer));
        return entity;
    }

} // namespace

GameOverMenu::GameOverMenu(FontManager& fonts, int finalScore, bool victory)
    : fonts_(fonts), finalScore_(finalScore), victory_(victory)
{}

void GameOverMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundRect_ = createSemiTransparentBackground(registry);

    std::string titleStr = victory_ ? "VICTORY!" : "GAME OVER";
    sf::Color titleColor = victory_ ? sf::Color::Green : sf::Color::Red;
    titleText_           = createCenteredText(registry, 150.0F, titleStr, 72, titleColor, 101);

    scoreText_ =
        createCenteredText(registry, 250.0F, "Score: " + std::to_string(finalScore_), 36, sf::Color::White, 101);

    retryButton_ =
        createCenteredButton(registry, 350.0F, "Retry", sf::Color(50, 150, 50), [this]() { onRetryClicked(); }, 101);

    quitButton_ =
        createCenteredButton(registry, 430.0F, "Quit", sf::Color(150, 50, 50), [this]() { onQuitClicked(); }, 101);

    Logger::instance().info("[GameOverMenu] Created game over screen");
}

void GameOverMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundRect_)) {
        registry.destroyEntity(backgroundRect_);
    }
    if (registry.isAlive(titleText_)) {
        registry.destroyEntity(titleText_);
    }
    if (registry.isAlive(scoreText_)) {
        registry.destroyEntity(scoreText_);
    }
    if (registry.isAlive(retryButton_)) {
        registry.destroyEntity(retryButton_);
    }
    if (registry.isAlive(quitButton_)) {
        registry.destroyEntity(quitButton_);
    }

    Logger::instance().info("[GameOverMenu] Destroyed game over screen");
}

bool GameOverMenu::isDone() const
{
    return done_;
}

void GameOverMenu::handleEvent(Registry& registry, const sf::Event& event)
{
    (void) registry;
    (void) event;
}

void GameOverMenu::render(Registry& registry, Window& window)
{
    static int renderCount = 0;
    if (renderCount++ % 60 == 0) {
        Logger::instance().info("[GameOverMenu] Rendering frame " + std::to_string(renderCount));
    }

    if (registry.isAlive(backgroundRect_) && registry.has<BoxComponent>(backgroundRect_)) {
        const auto& box       = registry.get<BoxComponent>(backgroundRect_);
        const auto& transform = registry.get<TransformComponent>(backgroundRect_);

        sf::RectangleShape rect(sf::Vector2f{box.width, box.height});
        rect.setPosition(sf::Vector2f{transform.x, transform.y});
        rect.setFillColor(box.fillColor);
        window.draw(rect);
    }

    if (registry.isAlive(titleText_) && registry.has<TextComponent>(titleText_)) {
        const auto& text      = registry.get<TextComponent>(titleText_);
        const auto& transform = registry.get<TransformComponent>(titleText_);

        const sf::Font* font = fonts_.get(text.fontId);
        if (font != nullptr) {
            sf::Text sfText(*font, text.content, text.characterSize);
            sfText.setPosition(sf::Vector2f{transform.x, transform.y});
            sfText.setFillColor(text.color);
            window.draw(sfText);
        }
    }

    if (registry.isAlive(scoreText_) && registry.has<TextComponent>(scoreText_)) {
        const auto& text      = registry.get<TextComponent>(scoreText_);
        const auto& transform = registry.get<TransformComponent>(scoreText_);

        const sf::Font* font = fonts_.get(text.fontId);
        if (font != nullptr) {
            sf::Text sfText(*font, text.content, text.characterSize);
            sfText.setPosition(sf::Vector2f{transform.x, transform.y});
            sfText.setFillColor(text.color);
            window.draw(sfText);
        }
    }

    if (registry.isAlive(retryButton_) && registry.has<BoxComponent>(retryButton_)) {
        const auto& box       = registry.get<BoxComponent>(retryButton_);
        const auto& transform = registry.get<TransformComponent>(retryButton_);
        const auto& button    = registry.get<ButtonComponent>(retryButton_);

        sf::RectangleShape rect(sf::Vector2f{box.width, box.height});
        rect.setPosition(sf::Vector2f{transform.x, transform.y});
        rect.setFillColor(button.hovered ? box.focusColor : box.fillColor);
        rect.setOutlineColor(box.outlineColor);
        rect.setOutlineThickness(box.outlineThickness);
        window.draw(rect);

        const sf::Font* font = fonts_.get("ui");
        if (font != nullptr) {
            sf::Text label(*font, button.label, 24);
            label.setPosition(sf::Vector2f{transform.x + 70.0F, transform.y + 15.0F});
            label.setFillColor(sf::Color::White);
            window.draw(label);
        }
    }

    if (registry.isAlive(quitButton_) && registry.has<BoxComponent>(quitButton_)) {
        const auto& box       = registry.get<BoxComponent>(quitButton_);
        const auto& transform = registry.get<TransformComponent>(quitButton_);
        const auto& button    = registry.get<ButtonComponent>(quitButton_);

        sf::RectangleShape rect(sf::Vector2f{box.width, box.height});
        rect.setPosition(sf::Vector2f{transform.x, transform.y});
        rect.setFillColor(button.hovered ? box.focusColor : box.fillColor);
        rect.setOutlineColor(box.outlineColor);
        rect.setOutlineThickness(box.outlineThickness);
        window.draw(rect);

        const sf::Font* font = fonts_.get("ui");
        if (font != nullptr) {
            sf::Text label(*font, button.label, 24);
            label.setPosition(sf::Vector2f{transform.x + 75.0F, transform.y + 15.0F});
            label.setFillColor(sf::Color::White);
            window.draw(label);
        }
    }
}

void GameOverMenu::onRetryClicked()
{
    Logger::instance().info("[GameOverMenu] Retry clicked");
    result_ = Result::Retry;
    done_   = true;
}

void GameOverMenu::onQuitClicked()
{
    Logger::instance().info("[GameOverMenu] Quit clicked");
    result_ = Result::Quit;
    done_   = true;
}
