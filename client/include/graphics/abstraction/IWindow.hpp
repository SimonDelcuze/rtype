#pragma once

#include <string>
#include <functional>
#include "Common.hpp"
#include "Event.hpp"
#include "ISprite.hpp"
#include "IText.hpp"

class IWindow {
public:
    virtual ~IWindow() = default;

    virtual bool isOpen() const = 0;
    virtual Vector2u getSize() const = 0;
    virtual void close() = 0;
    
    virtual void pollEvents(const std::function<void(const Event&)>& handler) = 0;
    
    virtual void clear(const Color& color = Color::Black) = 0;
    virtual void display() = 0;
    
    virtual void draw(const ISprite& sprite) = 0;
    virtual void draw(const IText& text) = 0;
    virtual void draw(const Vector2f* vertices, std::size_t vertexCount, Color color, int type) = 0;
};
