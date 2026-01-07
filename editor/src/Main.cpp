#include "editor/ImGuiSFML.hpp"
#include "editor/LevelEditor.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/ContextSettings.hpp>

#include <optional>

int main()
{
    AssetIndex assets = loadAssetIndex("client/assets/assets.json", "client/assets/animations.json");
    LevelEditor editor(assets);

    sf::ContextSettings settings;
    settings.depthBits         = 24;
    settings.stencilBits       = 8;
    settings.majorVersion   = 2;
    settings.minorVersion   = 1;
    settings.attributeFlags = sf::ContextSettings::Attribute::Default;

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "R-Type Level Editor", sf::State::Windowed, settings);
    window.setVerticalSyncEnabled(true);

    ImGuiSFMLContext imgui;
    if (!imgui.init(window))
        return 1;

    sf::Clock clock;
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->getIf<sf::Event::Closed>())
                window.close();
            imgui.processEvent(*event);
        }

        float deltaSeconds = clock.restart().asSeconds();
        imgui.newFrame(window, deltaSeconds);

        editor.draw();

        window.clear(sf::Color(18, 18, 24));
        imgui.render();
        window.display();
    }

    imgui.shutdown();
    return 0;
}
