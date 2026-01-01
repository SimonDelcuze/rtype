#include "ClientRuntime.hpp"
#include "Logger.hpp"
#include "graphics/FontManager.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

#include <chrono>
#include <thread>

Window createMainWindow()
{
    return Window(Vector2u{1280u, 720u}, "R-Type");
}

void showErrorMessage(Window& window, const std::string& message, float displayTime)
{
    FontManager fontManager;
    if (!fontManager.has("ui"))
        fontManager.load("ui", "client/assets/fonts/ui.ttf");

    auto font = fontManager.get("ui");
    if (!font)
        return;

    GraphicsFactory factory;
    auto errorText = factory.createText();
    errorText->setFont(*font);
    errorText->setString(message);
    errorText->setCharacterSize(32);
    errorText->setFillColor(Color::Red);

    FloatRect bounds = errorText->getGlobalBounds();
    errorText->setOrigin(Vector2f{bounds.width / 2.0F, bounds.height / 2.0F});
    errorText->setPosition(Vector2f{640.0F, 360.0F});

    auto start = std::chrono::steady_clock::now();

    while (window.isOpen() && g_running) {
        auto now                             = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - start;
        if (elapsed.count() > displayTime) {
            break;
        }

        window.pollEvents([&](const Event& event) {
            if (event.type == EventType::Closed) {
                window.close();
            }
        });

        window.clear(Color{30, 30, 40});
        window.draw(*errorText);
        window.display();
    }
}
