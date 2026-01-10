#pragma once

#include "concurrency/ThreadSafeQueue.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/IMenu.hpp"
#include "ui/NotificationData.hpp"

#include <string>

class RegisterMenu : public IMenu
{
  public:
    struct Result
    {
        bool registered    = false;
        bool backToLogin   = false;
        bool exitRequested = false;
        std::uint32_t userId{0};
        std::string username;
    };

    RegisterMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn,
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
    void handleRegisterAttempt(Registry& registry);
    bool validatePassword(const std::string& password, std::string& errorMsg);

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection& lobbyConn_;
    ThreadSafeQueue<NotificationData>& broadcastQueue_;

    bool done_          = false;
    bool backToLogin_   = false;
    bool exitRequested_ = false;
    bool registered_    = false;
    bool isLoading_     = false;

    std::uint32_t userId_{0};
    std::string username_;

    float heartbeatTimer_{0.0F};
    int consecutiveFailures_{0};

    EntityId usernameInput_        = 0;
    EntityId passwordInput_        = 0;
    EntityId confirmPasswordInput_ = 0;
};
