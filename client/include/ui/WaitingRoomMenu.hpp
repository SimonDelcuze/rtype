#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/UdpSocket.hpp"
#include "ui/IMenu.hpp"

#include <atomic>

class WaitingRoomMenu : public IMenu
{
  public:
    enum class State
    {
        WaitingForClick,
        WaitingForPlayers,
        Countdown,
        Done
    };

    struct Result
    {
        bool ready = false;
    };

    WaitingRoomMenu(FontManager& fonts, TextureManager& textures, UdpSocket& socket, const IpEndpoint& server,
                    std::atomic<bool>& allReadyFlag, std::atomic<int>& countdownValueFlag,
                    std::atomic<bool>& gameStartFlag);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    void update(Registry& registry, float dt);
    Result getResult(Registry& registry) const;

    State getState() const
    {
        return state_;
    }

  private:
    void onReadyClicked();
    void updateDotAnimation(float dt);
    void updateWaitingText(Registry& registry);
    void updateCountdownFromServer(Registry& registry);
    void hideButton(Registry& registry);

    FontManager& fonts_;
    TextureManager& textures_;
    UdpSocket& socket_;
    const IpEndpoint& server_;
    std::atomic<bool>& allReadyFlag_;
    std::atomic<int>& countdownValueFlag_;
    std::atomic<bool>& gameStartFlag_;

    State state_ = State::WaitingForClick;
    bool done_   = false;

    EntityId readyButton_   = 0;
    EntityId waitingText_   = 0;
    EntityId countdownText_ = 0;

    float dotTimer_        = 0.0F;
    int dotCount_          = 1;
    int lastCountdownVal_  = -1;
    bool buttonClicked_    = false;
    float readyRetryTimer_ = 0.0F;
};
