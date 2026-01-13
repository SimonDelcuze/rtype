#pragma once

#include "graphics/FontManager.hpp"
#include "ui/IMenu.hpp"

#include <vector>

class PauseMenu : public IMenu
{
  public:
    enum class Result
    {
        None,
        Resume,
        Quit
    };

    explicit PauseMenu(FontManager& fonts);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult() const
    {
        return result_;
    }

  private:
    void onResumeClicked();
    void onQuitClicked();

    void renderRectangle(Registry& registry, EntityId entityId, Window& window);
    void renderText(Registry& registry, EntityId entityId, Window& window);
    void renderButton(Registry& registry, EntityId entityId, Window& window, float labelOffsetX, float labelOffsetY);

    FontManager& fonts_;

    Result result_ = Result::None;
    bool done_     = false;

    EntityId backgroundOverlay_ = 0;
    EntityId menuBox_           = 0;
    EntityId titleText_         = 0;
    EntityId resumeButton_      = 0;
    EntityId quitButton_        = 0;
};
