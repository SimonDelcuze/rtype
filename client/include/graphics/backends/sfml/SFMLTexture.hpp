#pragma once

#include "../../abstraction/ITexture.hpp"

#include <SFML/Graphics/Texture.hpp>

class SFMLTexture : public ITexture
{
  public:
    bool loadFromFile(const std::string& filename) override;
    bool create(unsigned int width, unsigned int height) override;
    Vector2u getSize() const override;
    void setRepeated(bool repeated) override;
    bool isRepeated() const override;
    void setSmooth(bool smooth) override;
    bool isSmooth() const override;

    const sf::Texture& getSFMLTexture() const;

  private:
    sf::Texture texture_;
};
