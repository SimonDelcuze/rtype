#pragma once

#include "Common.hpp"
#include "Event.hpp"
#include "ISprite.hpp"
#include "IText.hpp"
#include "graphics/ColorFilter.hpp"

#include <functional>
#include <string>

class IWindow
{
  public:
    virtual ~IWindow() = default;

    virtual bool isOpen() const      = 0;
    virtual Vector2u getSize() const = 0;
    virtual void close()             = 0;

    virtual void pollEvents(const std::function<void(const Event&)>& handler) = 0;

    virtual void clear(const Color& color = Color::Black) = 0;
    virtual void display()                                = 0;

    virtual void draw(const ISprite& sprite)                                                    = 0;
    virtual void draw(const IText& text)                                                        = 0;
    virtual void draw(const Vector2f* vertices, std::size_t vertexCount, Color color, int type) = 0;
    virtual void drawRectangle(Vector2f size, Vector2f position, float rotation, Vector2f scale, Color fillColor,
                               Color outlineColor, float outlineThickness)                      = 0;

    virtual void setColorFilter(ColorFilterMode mode) = 0;
    virtual ColorFilterMode getColorFilter() const    = 0;
};
