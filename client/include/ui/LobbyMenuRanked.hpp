#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/LobbyConnection.hpp"
#include "network/LobbyPackets.hpp"
#include "ui/IMenu.hpp"
#include "ui/NotificationData.hpp"
#include "ui/RoomWaitingMenuRanked.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class LobbyMenuRanked : public IMenu
{
  public:
    struct Result
    {
        bool success{false};
        bool exitRequested{false};
        bool backRequested{false};
        bool serverLost{false};
        std::uint32_t roomId{0};
        std::uint16_t gamePort{0};
        std::uint8_t expectedPlayerCount{0};
    };

    LobbyMenuRanked(FontManager& fonts, TextureManager& textures, const IpEndpoint& lobbyEndpoint,
                    ThreadSafeQueue<NotificationData>& broadcastQueue, const std::atomic<bool>& runningFlag,
                    LobbyConnection* sharedConnection = nullptr);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry&) const { return result_; }

  private:
    void refreshRooms();
    void onFindGameClicked();
    void onBackClicked();
    void updateStatus(Registry& registry, const std::string& text);
    void transitionToWaiting(Registry& registry);
    void destroyLobbyEntities(Registry& registry);
    void buildLayout(Registry& registry);

    enum class State
    {
        Loading,
        Idle,
        Finding,
        Joining,
        InRoom,
        Done
    };

    FontManager& fonts_;
    TextureManager& textures_;
    IpEndpoint lobbyEndpoint_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;
    const std::atomic<bool>& runningFlag_;
    std::unique_ptr<LobbyConnection> lobbyConnection_;
    LobbyConnection* sharedConnection_{nullptr};
    bool ownsConnection_{true};

    Result result_;
    State state_{State::Loading};

    std::vector<RoomInfo> rooms_;
    float requestTimer_{0.0F};
    float dotTimer_{0.0F};
    int dotCount_{1};

    EntityId background_{0};
    EntityId logo_{0};
    EntityId title_{0};
    EntityId status_{0};
    EntityId findBtn_{0};
    EntityId backBtn_{0};
    EntityId leftBoard_{0};
    EntityId rightBoard_{0};
    EntityId leftTitle_{0};
    EntityId rightTitle_{0};
    bool layoutBuilt_{false};

    std::unique_ptr<RoomWaitingMenuRanked> waitingMenu_;
    bool waitingMenuInit_{false};

    Registry* registry_{nullptr};
};
