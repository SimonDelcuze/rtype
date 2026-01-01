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
    } else if (event.getIf<sf::Event::FocusLost>()) {
        e.type = EventType::LostFocus;
    } else if (event.getIf<sf::Event::FocusGained>()) {
        e.type = EventType::GainedFocus;
    } else if (auto* text = event.getIf<sf::Event::TextEntered>()) {
        e.type = EventType::TextEntered;
        e.text.unicode = text->unicode;
    } else if (auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        e.type = EventType::KeyPressed;
        e.key.code = mapKey(keyPressed->code);
        e.key.alt = keyPressed->alt;
        e.key.control = keyPressed->control;
        e.key.shift = keyPressed->shift;
        e.key.system = keyPressed->system;
    } else if (auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        e.type = EventType::KeyReleased;
        e.key.code = mapKey(keyReleased->code);
        e.key.alt = keyReleased->alt;
        e.key.control = keyReleased->control;
        e.key.shift = keyReleased->shift;
        e.key.system = keyReleased->system;
    } else if (auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        e.type = EventType::MouseWheelScrolled;
        e.mouseWheelScroll.delta = mouseWheel->delta;
        e.mouseWheelScroll.x = mouseWheel->position.x;
        e.mouseWheelScroll.y = mouseWheel->position.y;
    } else if (auto* mouseBtn = event.getIf<sf::Event::MouseButtonPressed>()) {
        e.type = EventType::MouseButtonPressed;
        e.mouseButton.button = static_cast<MouseButton>(mouseBtn->button);
        e.mouseButton.x = mouseBtn->position.x;
        e.mouseButton.y = mouseBtn->position.y;
    } else if (auto* mouseBtnRel = event.getIf<sf::Event::MouseButtonReleased>()) {
        e.type = EventType::MouseButtonReleased;
        e.mouseButton.button = static_cast<MouseButton>(mouseBtnRel->button);
        e.mouseButton.x = mouseBtnRel->position.x;
        e.mouseButton.y = mouseBtnRel->position.y;
    } else if (auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
        e.type = EventType::MouseMoved;
        e.mouseMove.x = mouseMove->position.x;
        e.mouseMove.y = mouseMove->position.y;
    } else if (event.getIf<sf::Event::MouseEntered>()) {
        e.type = EventType::MouseEntered;
    } else if (event.getIf<sf::Event::MouseLeft>()) {
        e.type = EventType::MouseLeft;
    }
    
    return e;
}
