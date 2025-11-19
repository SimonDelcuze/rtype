#pragma once

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <functional>
#include <string>

class Window
{
  public:
    Window(const sf::VideoMode& mode, const std::string& title, bool vsync = true);

    bool isOpen() const;
    void close();

    void pollEvents(const std::function<void(const sf::Event&)>& handler);

    void clear(const sf::Color& color = sf::Color::Black);
    void display();
    void draw(const sf::Drawable& drawable);

    sf::RenderWindow& raw();

  private:
    sf::RenderWindow window_;
};
