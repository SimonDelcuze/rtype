#pragma once

#include "../../abstraction/ISprite.hpp"
#include <SFML/Graphics/Sprite.hpp>

class SFMLSprite : public ISprite {
public:
    SFMLSprite();
    void setTexture(const ITexture& texture, bool resetRect = false) override;
    void setTextureRect(const IntRect& rect) override;
    void setPosition(const Vector2f& position) override;
    void setRotation(float angle) override;
    void setScale(const Vector2f& factor) override;
    void setOrigin(const Vector2f& origin) override;
    void setColor(const Color& color) override;
    
    Vector2f getPosition() const override;
    float getRotation() const override;
    Vector2f getScale() const override;
    Vector2f getOrigin() const override;
    FloatRect getGlobalBounds() const override;
    void move(const Vector2f& offset) override;
    void rotate(float angle) override;
    void scale(const Vector2f& factor) override;

    const sf::Sprite& getSFMLSprite() const;

private:
    sf::Sprite sprite_;
};
