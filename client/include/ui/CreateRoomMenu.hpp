#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/IMenu.hpp"

#include <string>

class CreateRoomMenu : public IMenu
{
  public:
    struct Result
    {
        bool created         = false;
        bool cancelled       = false;
        std::string roomName = "My Room";
        std::string password;
        RoomVisibility visibility = RoomVisibility::Public;
    };

    CreateRoomMenu(FontManager& fonts, TextureManager& textures);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const
    {
        (void) registry;
        return result_;
    }

  private:
    void onCreateClicked();
    void onCancelClicked();
    void onTogglePassword();

    FontManager& fonts_;
    TextureManager& textures_;
    Result result_;
    bool done_{false};

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId roomNameInputEntity_{0};
    EntityId passwordInputEntity_{0};
    EntityId passwordToggleEntity_{0};
    EntityId createButtonEntity_{0};
    EntityId cancelButtonEntity_{0};

    bool passwordEnabled_{false};
};
