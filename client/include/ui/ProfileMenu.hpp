#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "graphics/Window.hpp"
#include "network/LobbyConnection.hpp"
#include "ui/IMenu.hpp"

#include <cstdint>
#include <optional>
#include <string>

struct UserProfile
{
    std::uint32_t userId;
    std::string username;
    std::uint32_t gamesPlayed;
    std::uint32_t wins;
    std::uint32_t losses;
    std::uint64_t totalScore;
};

class ProfileMenu : public IMenu
{
  public:
    ProfileMenu(FontManager& fonts, TextureManager& textures, LobbyConnection& lobbyConn, const std::string& username,
                std::uint32_t userId);

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    bool isBackRequested() const
    {
        return backRequested_;
    }

  private:
    void fetchStats();
    void updateStatsDisplay(Registry& registry);
    float calculateWinRate() const;

    FontManager& fonts_;
    TextureManager& textures_;
    LobbyConnection& lobbyConn_;
    std::string username_;
    std::uint32_t userId_;

    bool done_          = false;
    bool backRequested_ = false;
    bool statsLoaded_   = false;

    UserProfile profile_;

    EntityId backgroundEntity_{0};
    EntityId titleEntity_{0};
    EntityId usernameEntity_{0};
    EntityId userIdEntity_{0};
    EntityId gamesPlayedEntity_{0};
    EntityId winsEntity_{0};
    EntityId lossesEntity_{0};
    EntityId winRateEntity_{0};
    EntityId totalScoreEntity_{0};
    EntityId backButtonEntity_{0};
};
