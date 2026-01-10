#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/IMenu.hpp"
#include "ui/NotificationData.hpp"

#include <string>

class LoginMenu : public IMenu
{
  public:
    struct Result
    {
        bool authenticated = false;
        bool openRegister  = false;
        bool backRequested = false;
        bool exitRequested = false;
        std::uint32_t userId{0};
        std::string username;
        std::string token;
    };

    LoginMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn,
              ThreadSafeQueue<NotificationData>& broadcastQueue);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void update(Registry& registry, float dt) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const;
    void setError(Registry& registry, const std::string& message);
    void reset();

  private:
    void handleLoginAttempt(Registry& registry);

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection& lobbyConn_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;

    bool done_          = false;
    bool openRegister_  = false;
    bool backRequested_ = false;
    bool exitRequested_ = false;
    bool authenticated_ = false;
    bool isLoading_     = false;

    std::uint32_t userId_{0};
    std::string username_;
    std::string token_;

    float heartbeatTimer_{0.0F};
    int consecutiveFailures_{0};

    EntityId usernameInput_ = 0;
    EntityId passwordInput_ = 0;
};
