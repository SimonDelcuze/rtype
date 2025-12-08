#pragma once

#include "graphics/FontManager.hpp"
#include "network/UdpSocket.hpp"

#include <SFML/Graphics.hpp>
#include <string>

class ConnectionMenu
{
  public:
    ConnectionMenu(FontManager& fonts);

    void handleEvent(const sf::Event& event);
    void render(sf::RenderWindow& window);

    bool isDone() const
    {
        return done_;
    }
    IpEndpoint getServerEndpoint() const;

  private:
    void handleTextInput(char c);
    void handleBackspace();
    void handleMouseClick(int x, int y);
    void connectToServer();
    void useDefaultServer();

    void drawLabels(sf::RenderWindow& window);
    void drawInputs(sf::RenderWindow& window);
    void drawButtons(sf::RenderWindow& window);

    bool isPointInRect(int x, int y, const sf::RectangleShape& rect) const;

    const sf::Font* font_;

    bool done_;
    bool useDefault_;

    std::string ipInput_;
    std::string portInput_;
    enum class ActiveField
    {
        None,
        IP,
        Port
    };
    ActiveField activeField_;

    sf::RectangleShape ipBox_;
    sf::RectangleShape portBox_;
    sf::RectangleShape connectButton_;
    sf::RectangleShape defaultButton_;
};
