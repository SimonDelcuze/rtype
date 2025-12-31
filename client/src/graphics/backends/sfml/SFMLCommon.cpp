#include "graphics/backends/sfml/SFMLCommon.hpp"
#include <iostream>

Event fromSFML(const sf::Event& event) {
    Event e;
    e.type = EventType::Count; 

    auto mapKey = [](sf::Keyboard::Key key) -> KeyCode {
        return static_cast<KeyCode>(key);
    };

    if (event.getIf<sf::Event::Closed>()) {
         e.type = EventType::Closed;
    } else if (auto* resized = event.getIf<sf::Event::Resized>()) {
        e.type = EventType::Resized;
        e.size = {resized->size.x, resized->size.y};
    } else if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        e.type = EventType::KeyPressed;
        e.key.code = mapKey(keyPressed->code);
        e.key.alt = keyPressed->alt;
        e.key.control = keyPressed->control;
        e.key.shift = keyPressed->shift;
        e.key.system = keyPressed->system;
    } 
    
    return e;
}
