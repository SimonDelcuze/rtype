#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/RoomType.hpp"
#include "ui/IMenu.hpp"

class ModeSelectMenu : public IMenu
{
  public:
    struct Result
    {
        bool backRequested{false};
        bool exitRequested{false};
        bool confirmed{false};
        RoomType selected{RoomType::Quickplay};
    };

    ModeSelectMenu(FontManager& fonts, TextureManager& textures);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry&) const { return result_; }

  private:
    FontManager& fonts_;
    TextureManager& textures_;
    bool done_{false};
    Result result_{};

    EntityId background_{0};
    EntityId logo_{0};
    EntityId title_{0};
    EntityId quickBtn_{0};
    EntityId rankedBtn_{0};
    EntityId backBtn_{0};
};
