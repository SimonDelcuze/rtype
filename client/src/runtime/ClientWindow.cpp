#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "graphics/FontManager.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
Window createMainWindow()
{
    return Window(sf::VideoMode({1280u, 720u}), "R-Type", true);
}

void showErrorMessage(Window& window, const std::string& message, float displayTime)
{
    FontManager fontManager;
    if (!fontManager.has("ui"))
        fontManager.load("ui", "client/assets/fonts/ui.ttf");

    const sf::Font* font = fontManager.get("ui");
    if (!font)
        return;

    sf::Text errorText(*font, message, 32);
    errorText.setFillColor(sf::Color::Red);

    sf::FloatRect bounds = errorText.getLocalBounds();
    errorText.setOrigin(sf::Vector2f{bounds.size.x / 2.0F, bounds.size.y / 2.0F});
    errorText.setPosition(sf::Vector2f{640.0F, 360.0F});

    sf::Clock clock;
    while (window.isOpen() && clock.getElapsedTime().asSeconds() < displayTime && g_running) {
        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
        });

        window.clear(sf::Color(30, 30, 40));
        window.draw(errorText);
        window.display();
    }
}
