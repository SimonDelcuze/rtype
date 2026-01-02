#pragma once

#include "graphics/abstraction/Common.hpp"
#include "graphics/abstraction/ISprite.hpp"
#include "graphics/abstraction/IText.hpp"
#include "graphics/abstraction/IWindow.hpp"

#include <functional>
#include <memory>
#include <string>

class Window
{
  public:
    Window(const Vector2u& size, const std::string& title);

    bool isOpen() const;
    Vector2u getSize() const;
    void close();

    void pollEvents(const std::function<void(const Event&)>& handler);

    void clear(const Color& color = Color::Black);
    void display();
    void draw(const ISprite& sprite);
    void draw(const IText& text);
    void draw(const Vector2f* vertices, std::size_t vertexCount, Color color, int type);
    void drawRectangle(Vector2f size, Vector2f position, float rotation, Vector2f scale, Color fillColor,
                       Color outlineColor, float outlineThickness);

    std::shared_ptr<IWindow> getNativeWindow() const;

  private:
    std::shared_ptr<IWindow> window_;
};
