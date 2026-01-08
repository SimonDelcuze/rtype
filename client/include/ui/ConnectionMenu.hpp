#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/IMenu.hpp"

#include <chrono>
#include <string>

class ConnectionMenu : public IMenu
{
  public:
    struct Result
    {
        bool connected     = false;
        bool useDefault    = false;
        bool openSettings  = false;
        bool exitRequested = false;
        std::string ip;
        std::string port;
    };

    ConnectionMenu(FontManager& fonts, TextureManager& textures, std::string initialError = "");

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const;
    void setError(Registry& registry, const std::string& message);
    void reset();

  private:
    void showConnectingText(Registry& registry);
    void updateConnectingText(Registry& registry);

    FontManager& fonts_;
    TextureManager& textures_;
    std::string initialError_;
    bool done_               = false;
    bool useDefault_         = false;
    bool openSettings_       = false;
    bool exitRequested_      = false;
    bool connecting_         = false;
    EntityId errorText_      = 0;
    EntityId connectingText_ = 0;
    std::chrono::steady_clock::time_point connectingStartTime_;
    int dotCount_ = 1;
};
