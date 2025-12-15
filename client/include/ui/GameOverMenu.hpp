#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/IMenu.hpp"

class GameOverMenu : public IMenu
{
  public:
    enum class Result
    {
        None,
        Retry,
        Quit
    };

    GameOverMenu(FontManager& fonts, int finalScore, bool victory);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const sf::Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult() const
    {
        return result_;
    }

  private:
    void onRetryClicked();
    void onQuitClicked();

    FontManager& fonts_;
    int finalScore_;
    bool victory_;

    Result result_ = Result::None;
    bool done_     = false;

    EntityId titleText_      = 0;
    EntityId scoreText_      = 0;
    EntityId retryButton_    = 0;
    EntityId quitButton_     = 0;
    EntityId backgroundRect_ = 0;
};
