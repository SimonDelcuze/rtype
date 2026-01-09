#include "ui/ProfileMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"
#include "network/StatsPackets.hpp"

#include <iomanip>
#include <sstream>

namespace
{
    EntityId createLabel(Registry& registry, float x, float y, const std::string& text, std::uint32_t fontSize = 20)
    {
        EntityId entity = registry.createEntity();
        TransformComponent transform;
        transform.x = x;
        transform.y = y;
        registry.emplace<TransformComponent>(entity, transform);

        TextComponent textComp = TextComponent::create("ui", fontSize, Color(255, 255, 255));
        textComp.content       = text;
        registry.emplace<TextComponent>(entity, textComp);

        return entity;
    }

    EntityId createButton(Registry& registry, float x, float y, const std::string& label, Color fill,
                          std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        TransformComponent transform;
        transform.x = x;
        transform.y = y;
        registry.emplace<TransformComponent>(entity, transform);

        BoxComponent box;
        box.width      = 150.0F;
        box.height     = 40.0F;
        box.fillColor  = fill;
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

} // namespace

ProfileMenu::ProfileMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn,
                         const std::string& username, std::uint32_t userId)
    : fonts_(fonts), textures_(textures), lobbyConn_(lobbyConn), username_(username), userId_(userId)
{
    profile_.userId      = userId;
    profile_.username    = username;
    profile_.gamesPlayed = 0;
    profile_.wins        = 0;
    profile_.losses      = 0;
    profile_.totalScore  = 0;
}

void ProfileMenu::create(Registry& registry)
{
    if (!fonts_.has("ui"))
        fonts_.load("ui", "client/assets/fonts/ui.ttf");

    backgroundEntity_ = registry.createEntity();
    TransformComponent bgTransform;
    bgTransform.x = 0.0F;
    bgTransform.y = 0.0F;
    registry.emplace<TransformComponent>(backgroundEntity_, bgTransform);

    BoxComponent bgBox;
    bgBox.width     = 1280.0F;
    bgBox.height    = 720.0F;
    bgBox.fillColor = Color(0, 0, 0, 180);
    registry.emplace<BoxComponent>(backgroundEntity_, bgBox);

    EntityId panelEntity = registry.createEntity();
    TransformComponent panelTransform;
    panelTransform.x = 340.0F;
    panelTransform.y = 150.0F;
    registry.emplace<TransformComponent>(panelEntity, panelTransform);

    BoxComponent panelBox;
    panelBox.width     = 600.0F;
    panelBox.height    = 420.0F;
    panelBox.fillColor = Color(30, 30, 40);
    registry.emplace<BoxComponent>(panelEntity, panelBox);

    titleEntity_ = createLabel(registry, 500.0F, 180.0F, "USER PROFILE", 32);

    usernameEntity_ = createLabel(registry, 400.0F, 240.0F, "Username: " + username_, 24);

    userIdEntity_ = createLabel(registry, 400.0F, 280.0F, "User ID: " + std::to_string(userId_), 20);

    gamesPlayedEntity_ = createLabel(registry, 400.0F, 320.0F, "Games Played: Loading...", 20);
    winsEntity_        = createLabel(registry, 400.0F, 350.0F, "Wins: Loading...", 20);
    lossesEntity_      = createLabel(registry, 400.0F, 380.0F, "Losses: Loading...", 20);
    winRateEntity_     = createLabel(registry, 400.0F, 410.0F, "Win Rate: Loading...", 20);
    totalScoreEntity_  = createLabel(registry, 400.0F, 440.0F, "Total Score: Loading...", 20);

    backButtonEntity_ = createButton(registry, 565.0F, 500.0F, "Back", Color(100, 100, 100), [this]() {
        Logger::instance().info("[ProfileMenu] Back clicked");
        backRequested_ = true;
        done_          = true;
    });

    fetchStats();
    updateStatsDisplay(registry);
}

void ProfileMenu::destroy(Registry& registry)
{
    registry.clear();
}

bool ProfileMenu::isDone() const
{
    return done_;
}

void ProfileMenu::handleEvent(Registry& /* registry */, const Event& /* event */) {}

void ProfileMenu::render(Registry& /* registry */, Window& /* window */) {}

void ProfileMenu::fetchStats()
{
    Logger::instance().info("[ProfileMenu] Fetching stats for user " + username_);

    auto statsData = lobbyConn_.getStats();
    if (!statsData.has_value()) {
        Logger::instance().warn("[ProfileMenu] Failed to get stats from server");
        return;
    }

    profile_.userId      = statsData->userId;
    profile_.gamesPlayed = statsData->gamesPlayed;
    profile_.wins        = statsData->wins;
    profile_.losses      = statsData->losses;
    profile_.totalScore  = statsData->totalScore;

    statsLoaded_ = true;

    Logger::instance().info("[ProfileMenu] Stats loaded: games=" + std::to_string(profile_.gamesPlayed) +
                            ", wins=" + std::to_string(profile_.wins));
}

void ProfileMenu::updateStatsDisplay(Registry& registry)
{
    if (!statsLoaded_)
        return;

    if (gamesPlayedEntity_ != 0 && registry.has<TextComponent>(gamesPlayedEntity_)) {
        auto& text   = registry.get<TextComponent>(gamesPlayedEntity_);
        text.content = "Games Played: " + std::to_string(profile_.gamesPlayed);
    }

    if (winsEntity_ != 0 && registry.has<TextComponent>(winsEntity_)) {
        auto& text   = registry.get<TextComponent>(winsEntity_);
        text.content = "Wins: " + std::to_string(profile_.wins);
    }

    if (lossesEntity_ != 0 && registry.has<TextComponent>(lossesEntity_)) {
        auto& text   = registry.get<TextComponent>(lossesEntity_);
        text.content = "Losses: " + std::to_string(profile_.losses);
    }

    if (winRateEntity_ != 0 && registry.has<TextComponent>(winRateEntity_)) {
        auto& text = registry.get<TextComponent>(winRateEntity_);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << calculateWinRate() << "%";
        text.content = "Win Rate: " + oss.str();
    }

    if (totalScoreEntity_ != 0 && registry.has<TextComponent>(totalScoreEntity_)) {
        auto& text   = registry.get<TextComponent>(totalScoreEntity_);
        text.content = "Total Score: " + std::to_string(profile_.totalScore);
    }
}

float ProfileMenu::calculateWinRate() const
{
    if (profile_.gamesPlayed == 0)
        return 0.0F;
    return (static_cast<float>(profile_.wins) / static_cast<float>(profile_.gamesPlayed)) * 100.0F;
}
