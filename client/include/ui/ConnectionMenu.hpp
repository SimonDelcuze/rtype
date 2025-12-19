#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/IMenu.hpp"

#include <string>

class ConnectionMenu : public IMenu
{
  public:
    struct Result
    {
        bool connected    = false;
        bool useDefault   = false;
        bool openSettings = false;
        std::string ip;
        std::string port;
    };

    ConnectionMenu(FontManager& fonts, TextureManager& textures, std::string initialError = "");

    void create(Registry& registry) override;
    void destroy(Registry& registry) override;
    bool isDone() const override;
    void handleEvent(Registry& registry, const sf::Event& event) override;
    void render(Registry& registry, Window& window) override;

    Result getResult(Registry& registry) const;
    void setError(Registry& registry, const std::string& message);
    void reset();

  private:
    FontManager& fonts_;
    TextureManager& textures_;
    std::string initialError_;
    bool done_          = false;
    bool useDefault_    = false;
    bool openSettings_  = false;
    EntityId errorText_ = 0;
};
