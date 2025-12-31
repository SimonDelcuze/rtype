#pragma once

#include <string>
#include "Common.hpp"
#include "IFont.hpp"

class IText {
public:
    virtual ~IText() = default;

    virtual void setFont(const IFont& font) = 0;
    virtual void setString(const std::string& string) = 0;
    virtual void setCharacterSize(unsigned int size) = 0;
    virtual void setFillColor(const Color& color) = 0;
    virtual void setOutlineColor(const Color& color) = 0;
    virtual void setOutlineThickness(float thickness) = 0;
    virtual void setPosition(const Vector2f& position) = 0;
    virtual void setRotation(float angle) = 0;
    virtual void setScale(const Vector2f& factor) = 0;
    virtual void setOrigin(const Vector2f& origin) = 0;

    virtual std::string getString() const = 0;
    virtual Vector2f getPosition() const = 0;
    virtual FloatRect getGlobalBounds() const = 0;
};
