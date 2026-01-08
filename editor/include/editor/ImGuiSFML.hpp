#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

class ImGuiSFMLContext
{
  public:
    bool init(sf::RenderWindow& window);
    void shutdown();
    void processEvent(const sf::Event& event);
    void newFrame(sf::RenderWindow& window, float deltaSeconds);
    void render();

  private:
    bool initialized_ = false;
};
