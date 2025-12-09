#pragma once

#include "graphics/FontManager.hpp"
#include "graphics/TextureManager.hpp"
#include "ui/IMenu.hpp"

class ConnectionMenu : public IMenu
{
  public:
    struct Result
    {
        bool connected  = false;
        bool useDefault = false;
        std::string ip;
        std::string port;
    };

    ConnectionMenu(FontManager& fonts, TextureManager& textures);

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
    bool done_          = false;
    bool useDefault_    = false;
    EntityId errorText_ = 0;
};
