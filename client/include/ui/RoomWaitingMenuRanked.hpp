#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyConnection.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/IMenu.hpp"

#include <string>
#include <vector>

class RoomWaitingMenuRanked : public IMenu
{
  public:
    struct PlayerRow
    {
        std::uint32_t playerId{0};
        std::string name;
        std::string rankName{"Unknown"};
        int elo{0};
        bool isReady{false};
    };

    struct Result
    {
        bool startGame{false};
        bool leaveRoom{false};
        bool serverLost{false};
        std::uint32_t roomId{0};
        std::uint16_t gamePort{0};
        std::uint8_t expectedPlayerCount{0};
    };

    RoomWaitingMenuRanked(FontManager& fonts, TextureManager& textures, std::uint32_t roomId,
                          const std::string& roomName, std::uint16_t gamePort, LobbyConnection* lobbyConnection);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry&) const
    {
        return result_;
    }

  private:
    void buildChrome(Registry& registry);
    void buildPlayerList(Registry& registry);
    void buildChatUI(Registry& registry);
    void refreshPlayers(Registry& registry);
    void onSendChatClicked(Registry& registry);

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection* lobbyConnection_{nullptr};

    Result result_;
    std::uint32_t roomId_{0};
    std::string roomName_;
    std::uint16_t gamePort_{0};

    EntityId background_{0};
    EntityId logo_{0};
    EntityId title_{0};
    EntityId playerCount_{0};
    EntityId status_{0};
    EntityId readyButton_{0};
    EntityId readyButtonText_{0};
    EntityId timerLabel_{0};

    std::vector<PlayerRow> players_;
    std::vector<EntityId> playerEntities_;

    EntityId chatBg_{0};
    EntityId chatInput_{0};
    EntityId chatSend_{0};
    std::vector<std::string> chatHistory_;
    std::vector<EntityId> chatEntities_;
    std::vector<EntityId> chatMessageEntities_;
    constexpr static std::size_t kMaxChatMessages = 12;

    float updateTimer_{0.0F};
    constexpr static float kUpdateInterval = 1.0F;

    int consecutiveFailures_{0};
    bool isRefreshing_{false};
    bool isReady_{false};
};
