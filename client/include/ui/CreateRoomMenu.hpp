#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/IMenu.hpp"
#include "ui/RoomDifficulty.hpp"

#include <array>
#include <cstdint>
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
        RoomVisibility visibility   = RoomVisibility::Public;
        RoomDifficulty difficulty   = RoomDifficulty::Noob;
        float enemyMultiplier       = 1.0F;
        float playerSpeedMultiplier = 1.0F;
        float scoreMultiplier       = 1.0F;
        std::uint8_t playerLives    = 3;
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
    void setDifficulty(RoomDifficulty difficulty);
    void updateDifficultyUI(Registry& registry);

    struct ConfigRow
    {
        EntityId label{0};
        EntityId input{0};
        EntityId suffix{0};
    };
    ConfigRow createConfigRow(Registry& registry, float x, float y, const std::string& label, const std::string& value);
    void destroyConfigRow(Registry& registry, const ConfigRow& row);
    void setInputValue(Registry& registry, EntityId inputId, const std::string& value);
    float readPercentField(Registry& registry, EntityId inputId, float minVal, float maxVal, float fallback);
    std::uint8_t readLivesField(Registry& registry, EntityId inputId, std::uint8_t fallback);

    FontManager& fonts_;
    TextureManager& textures_;
    Result result_;
    bool done_{false};

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId roomNameLabelEntity_{0};
    EntityId roomNameInputEntity_{0};
    EntityId passwordLabelEntity_{0};
    EntityId passwordInputEntity_{0};
    EntityId passwordToggleEntity_{0};
    EntityId createButtonEntity_{0};
    EntityId cancelButtonEntity_{0};
    EntityId difficultyTitleEntity_{0};
    EntityId configTitleEntity_{0};
    std::array<EntityId, 4> difficultyButtons_{};
    ConfigRow enemyRow_{};
    ConfigRow playerRow_{};
    ConfigRow scoreRow_{};
    ConfigRow livesRow_{};

    bool passwordEnabled_{false};
};
