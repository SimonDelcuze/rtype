#include "ui/GameOverMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

#include <algorithm>

namespace
{
    EntityId createBackground(Registry& registry, bool victory)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 0.0F;
        transform.y     = 0.0F;

        Color color = victory ? Color{0, 0, 0, 255} : Color{0, 0, 0, 180};
        auto box    = BoxComponent::create(1280.0F, 720.0F, color, color);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<LayerComponent>(entity, LayerComponent::create(100));
        return entity;
    }

    EntityId createCenteredText(Registry& registry, float y, const std::string& content, unsigned int size, Color color,
                                int layer = 101)
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
                                  std::function<void()> onClick, int layer = 101)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 540.0F;
        transform.y     = y;

        auto box       = BoxComponent::create(200.0F, 60.0F, fill,
                                              Color{static_cast<uint8_t>(fill.r + 40), static_cast<uint8_t>(fill.g + 40),
                                              static_cast<uint8_t>(fill.b + 40)});
        box.focusColor = Color{100, 200, 255};
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        registry.emplace<LayerComponent>(entity, LayerComponent::create(layer));
        return entity;
    }

    EntityId createLeaderboardEntry(Registry& registry, float y, int rank, std::uint32_t playerId, int score,
                                    const std::string& playerName)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = 340.0F;
        transform.y     = y;

        std::string nameDisplay = playerName.empty() ? ("Player " + std::to_string(playerId)) : playerName;
        std::string content =
            "#" + std::to_string(rank) + "   " + nameDisplay + "   -   " + std::to_string(score) + " pts";

        Color color  = (rank == 1)   ? Color{255, 215, 0}
                       : (rank == 2) ? Color{192, 192, 192}
                       : (rank == 3) ? Color{205, 127, 50}
                                     : Color::White;
        auto text    = TextComponent::create("ui", 28, color);
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        registry.emplace<LayerComponent>(entity, LayerComponent::create(101));
        return entity;
    }

} // namespace

GameOverMenu::GameOverMenu(FontManager& fonts, const std::vector<PlayerScoreEntry>& playerScores, bool victory)
    : fonts_(fonts), playerScores_(playerScores), victory_(victory)
{}

void GameOverMenu::create(Registry& registry)
{
    if (!fonts_.has("ui")) {
        fonts_.load("ui", "client/assets/fonts/ui.ttf");
    }

    backgroundRect_ = createBackground(registry, victory_);

    std::string titleStr = victory_ ? "WIN" : "GAME OVER";
    Color titleColor     = victory_ ? Color::Green : Color::Red;
    titleText_           = createCenteredText(registry, 80.0F, titleStr, 72, titleColor, 101);

    std::vector<PlayerScoreEntry> sortedScores = playerScores_;
    std::sort(sortedScores.begin(), sortedScores.end(),
              [](const PlayerScoreEntry& a, const PlayerScoreEntry& b) { return a.score > b.score; });

    auto headerText = createCenteredText(registry, 160.0F, "LEADERBOARD", 36, Color::White, 101);
    leaderboardTexts_.push_back(headerText);

    float startY           = 210.0F;
    float entryHeight      = 45.0F;
    std::size_t maxEntries = std::min(sortedScores.size(), static_cast<std::size_t>(5));

    for (std::size_t i = 0; i < maxEntries; ++i) {
        EntityId entryId =
            createLeaderboardEntry(registry, startY + (i * entryHeight), static_cast<int>(i + 1),
                                   sortedScores[i].playerId, sortedScores[i].score, sortedScores[i].playerName);
        leaderboardTexts_.push_back(entryId);
    }

    float buttonY = startY + (maxEntries * entryHeight) + 40.0F;
    if (buttonY < 400.0F) {
        buttonY = 400.0F;
    }

    retryButton_ =
        createCenteredButton(registry, buttonY, "Retry", Color{50, 150, 50}, [this]() { onRetryClicked(); }, 101);

    quitButton_ =
        createCenteredButton(registry, buttonY + 80.0F, "Quit", Color{150, 50, 50}, [this]() { onQuitClicked(); }, 101);

    Logger::instance().info("[GameOverMenu] Created game over screen with " + std::to_string(sortedScores.size()) +
                            " player(s)");
}

void GameOverMenu::destroy(Registry& registry)
{
    if (registry.isAlive(backgroundRect_)) {
        registry.destroyEntity(backgroundRect_);
    }
    if (registry.isAlive(titleText_)) {
        registry.destroyEntity(titleText_);
    }
    for (EntityId textId : leaderboardTexts_) {
        if (registry.isAlive(textId)) {
            registry.destroyEntity(textId);
        }
    }
    leaderboardTexts_.clear();
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

void GameOverMenu::handleEvent(Registry&, const Event&) {}

void GameOverMenu::render(Registry& registry, Window& window)
{
    static int renderCount = 0;
    if (renderCount++ % 60 == 0) {
        Logger::instance().info("[GameOverMenu] Rendering frame " + std::to_string(renderCount));
    }

    renderRectangle(registry, backgroundRect_, window);
    renderText(registry, titleText_, window);

    for (EntityId textId : leaderboardTexts_) {
        renderText(registry, textId, window);
    }

    renderButton(registry, retryButton_, window, 70.0F, 15.0F);
    renderButton(registry, quitButton_, window, 75.0F, 15.0F);
}

void GameOverMenu::renderRectangle(Registry& registry, EntityId entityId, Window& window)
{
    if (registry.isAlive(entityId) && registry.has<BoxComponent>(entityId)) {
        (void) window;
    }
}

void GameOverMenu::renderText(Registry& registry, EntityId entityId, Window& window)
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

void GameOverMenu::renderButton(Registry& registry, EntityId entityId, Window& window, float labelOffsetX,
                                float labelOffsetY)
{
    if (registry.isAlive(entityId) && registry.has<BoxComponent>(entityId)) {
        const auto& transform = registry.get<TransformComponent>(entityId);
        const auto& button    = registry.get<ButtonComponent>(entityId);

        auto font = fonts_.get("ui");
        if (font != nullptr) {
            GraphicsFactory factory;
            auto label = factory.createText();
            label->setFont(*font);
            label->setString(button.label);
            label->setCharacterSize(24);
            label->setPosition(Vector2f{transform.x + labelOffsetX, transform.y + labelOffsetY});
            label->setFillColor(Color::White);
            window.draw(*label);
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
