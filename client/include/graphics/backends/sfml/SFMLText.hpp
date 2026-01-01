#pragma once

#include "../../abstraction/IText.hpp"
#include <SFML/Graphics/Text.hpp>

class SFMLText : public IText {
public:
    SFMLText();
    void setFont(const IFont& font) override;
    void setString(const std::string& string) override;
    void setCharacterSize(unsigned int size) override;
    void setFillColor(const Color& color) override;
    void setOutlineColor(const Color& color) override;
    void setOutlineThickness(float thickness) override;
    void setPosition(const Vector2f& position) override;
    void setRotation(float angle) override;
    void setScale(const Vector2f& factor) override;
    void setOrigin(const Vector2f& origin) override;

    unsigned int getCharacterSize() const override;

    std::string getString() const override;
    Vector2f getPosition() const override;
    FloatRect getGlobalBounds() const override;
    FloatRect getLocalBounds() const override;

    const sf::Text& getSFMLText() const;

private:
    sf::Text text_;
};
