#pragma once

#include "events/GameEvents.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/IMenu.hpp"

#include <vector>

class GameOverMenu : public IMenu
{
  public:
    enum class Result
    {
        None,
        Retry,
        Quit
    };

    GameOverMenu(FontManager& fonts, const std::vector<PlayerScoreEntry>& playerScores, bool victory);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
#include "graphics/abstraction/Event.hpp"

    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult() const
    {
        return result_;
    }

  private:
    void onRetryClicked();
    void onQuitClicked();

    void renderRectangle(Registry& registry, EntityId entityId, Window& window);
    void renderText(Registry& registry, EntityId entityId, Window& window);
    void renderButton(Registry& registry, EntityId entityId, Window& window, float labelOffsetX, float labelOffsetY);

    FontManager& fonts_;
    std::vector<PlayerScoreEntry> playerScores_;
    bool victory_;

    Result result_ = Result::None;
    bool done_     = false;

    EntityId titleText_      = 0;
    EntityId backgroundRect_ = 0;
    EntityId retryButton_    = 0;
    EntityId quitButton_     = 0;
    std::vector<EntityId> leaderboardTexts_;
};
