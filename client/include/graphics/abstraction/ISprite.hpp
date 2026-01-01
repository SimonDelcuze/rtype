#pragma once

#include "Common.hpp"
#include "ITexture.hpp"

class ISprite {
public:
    virtual ~ISprite() = default;

    virtual void setTexture(const ITexture& texture, bool resetRect = false) = 0;
    virtual void setTextureRect(const IntRect& rect) = 0;
    virtual void setPosition(const Vector2f& position) = 0;
    virtual void setRotation(float angle) = 0;
    virtual void setScale(const Vector2f& factor) = 0;
    virtual void setOrigin(const Vector2f& origin) = 0;
    virtual void setColor(const Color& color) = 0;
    virtual Color getColor() const = 0;
    virtual IntRect getTextureRect() const = 0;
    
    virtual Vector2f getPosition() const = 0;
    virtual float getRotation() const = 0;
    virtual Vector2f getScale() const = 0;
    virtual Vector2f getOrigin() const = 0;
    virtual FloatRect getGlobalBounds() const = 0;
    virtual void move(const Vector2f& offset) = 0;
    virtual void rotate(float angle) = 0;
    virtual void scale(const Vector2f& factor) = 0;
};
