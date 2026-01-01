#pragma once

#include "../../abstraction/IWindow.hpp"
#include <SFML/Graphics.hpp>
#include <memory>

class SFMLWindow : public IWindow {
public:
    SFMLWindow(unsigned int width, unsigned int height, const std::string& title);
    ~SFMLWindow() override = default;

    bool isOpen() const override;
    Vector2u getSize() const override;
    void close() override;
    void pollEvents(const std::function<void(const Event&)>& handler) override;
    void clear(const Color& color) override;
    void display() override;
    
    void draw(const ISprite& sprite) override;
    void draw(const IText& text) override;
    void draw(const Vector2f* vertices, std::size_t vertexCount, Color color, int type) override;

private:
    sf::RenderWindow window_;
};
