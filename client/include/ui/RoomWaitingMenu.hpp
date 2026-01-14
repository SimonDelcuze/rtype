#pragma once

#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/IMenu.hpp"
#include "ui/RoomDifficulty.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class RoomWaitingMenu : public IMenu
{
  public:
    struct Result
    {
        bool startGame  = false;
        bool leaveRoom  = false;
        bool serverLost = false;
        std::uint32_t roomId{0};
        std::uint16_t gamePort{0};
        std::uint8_t expectedPlayerCount{0};
        RoomDifficulty difficulty{RoomDifficulty::Noob};
        float enemyMultiplier{1.0F};
        float playerSpeedMultiplier{1.0F};
        float scoreMultiplier{1.0F};
        std::uint8_t playerLives{3};
    };

    struct PlayerInfo
    {
        std::uint32_t playerId{0};
        std::string name;
        bool isHost{false};
        bool isSpectator{false};
    };

    RoomWaitingMenu(FontManager& fonts, TextureManager& textures, std::uint32_t roomId, const std::string& roomName,
                    std::uint16_t gamePort, bool isHost, bool isRanked, LobbyConnection* lobbyConnection);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const
    {
        (void) registry;
        return result_;
    }

  private:
    void onStartGameClicked(Registry& registry);
    void onLeaveRoomClicked();
    void onKickPlayerClicked(std::uint32_t playerId);
    void updatePlayerList(Registry& registry);
    void onSendChatClicked(Registry& registry);
    void buildDifficultyUI(Registry& registry);
    void destroyDifficultyUI(Registry& registry);
    void buildChrome(Registry& registry);
    void buildControlButtons(Registry& registry);
    void buildChatUI(Registry& registry);
    void destroyChatUI(Registry& registry);
    void setDifficulty(RoomDifficulty difficulty);
    void updateDifficultyUI(Registry& registry);
    void setInputValue(Registry& registry, EntityId inputId, const std::string& value);
    void maybeSendRoomConfig();

    struct ConfigRow
    {
        EntityId label{0};
        EntityId input{0};
        EntityId upBtn{0};
        EntityId downBtn{0};
    };

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection* lobbyConnection_;
    Result result_;
    bool done_{false};

    std::uint32_t roomId_;
    std::string roomName_;
    std::uint16_t gamePort_;
    bool isHost_;
    bool isRanked_{false};
    std::vector<PlayerInfo> players_;

    EntityId backgroundEntity_{0};
    EntityId logoEntity_{0};
    EntityId titleEntity_{0};
    EntityId playerCountEntity_{0};
    EntityId startButtonEntity_{0};
    EntityId leaveButtonEntity_{0};
    std::vector<EntityId> playerTextEntities_;
    std::vector<EntityId> playerBadgeEntities_;
    std::vector<EntityId> kickButtonEntities_;
    EntityId chatBackgroundEntity_{0};
    EntityId chatInputField_{0};
    EntityId sendButtonEntity_{0};
    std::vector<EntityId> chatMessageEntities_;
    std::vector<std::string> chatHistory_;
    constexpr static std::size_t kMaxChatMessages = 12;

    EntityId difficultyTitleEntity_{0};
    EntityId configTitleEntity_{0};
    EntityId selectedDifficultyLabel_{0};
    std::array<EntityId, 4> difficultyButtons_{};
    std::array<EntityId, 4> difficultyIcons_{};
    ConfigRow enemyRow_{};
    ConfigRow playerRow_{};
    ConfigRow scoreRow_{};
    ConfigRow livesRow_{};
    RoomDifficulty difficulty_{RoomDifficulty::Noob};
    float enemyMultiplier_{1.0F};
    float playerSpeedMultiplier_{1.0F};
    float scoreMultiplier_{1.0F};
    std::uint8_t playerLives_{3};
    struct LastConfig
    {
        RoomDifficulty mode{RoomDifficulty::Noob};
        float enemy{0};
        float player{0};
        float score{0};
        std::uint8_t lives{0};
    } lastSentConfig_{};
    bool suppressSend_{false};

    float updateTimer_{0.0F};
    constexpr static float kUpdateInterval = 1.0F;

    int consecutiveFailures_{0};
    bool isRefreshingPlayers_{false};
    EntityId startingGameMessageEntity_{0};
    bool isStarting_{false};
};
