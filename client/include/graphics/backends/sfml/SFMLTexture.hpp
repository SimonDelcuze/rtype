#pragma once

#include "../../abstraction/ITexture.hpp"
#include <SFML/Graphics/Texture.hpp>

class SFMLTexture : public ITexture {
public:
    bool loadFromFile(const std::string& filename) override;
    Vector2u getSize() const override;
    void setRepeated(bool repeated) override;
    bool isRepeated() const override;
    void setSmooth(bool smooth) override;
    bool isSmooth() const override;

    const sf::Texture& getSFMLTexture() const;

private:
    sf::Texture texture_;
};
