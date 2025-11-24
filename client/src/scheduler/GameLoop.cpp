#include "scheduler/GameLoop.hpp"

#include <SFML/System/Clock.hpp>

int GameLoop::run(Window& window, Registry& registry)
{
    sf::Clock clock;

    while (window.isOpen()) {
        window.pollEvents([&](const sf::Event& event) {
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
        });

        const float deltaTime = clock.restart().asSeconds();

        window.clear();
        ClientScheduler::update(registry, deltaTime);
        window.display();
    }

    ClientScheduler::stop();
    return 0;
}
