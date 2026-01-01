#include "graphics/backends/sfml/SFMLWindow.hpp"

#include "graphics/backends/sfml/SFMLCommon.hpp"
#include "graphics/backends/sfml/SFMLSprite.hpp"
#include "graphics/backends/sfml/SFMLText.hpp"

#include <iostream>

SFMLWindow::SFMLWindow(unsigned int width, unsigned int height, const std::string& title)
    : window_(sf::VideoMode({width, height}), title)
{}

bool SFMLWindow::isOpen() const
{
    return window_.isOpen();
}

void SFMLWindow::close()
{
    window_.close();
}

Vector2u SFMLWindow::getSize() const
{
    return fromSFML(window_.getSize());
}

void SFMLWindow::pollEvents(const std::function<void(const Event&)>& handler)
{
    while (const std::optional<sf::Event> event = window_.pollEvent()) {
        handler(fromSFML(*event));
    }
}

void SFMLWindow::clear(const Color& color)
{
    window_.clear(toSFML(color));
}

void SFMLWindow::display()
{
    window_.display();
}

void SFMLWindow::draw(const ISprite& sprite)
{
    const auto* sfmlSprite = dynamic_cast<const SFMLSprite*>(&sprite);
    if (sfmlSprite) {
        window_.draw(sfmlSprite->getSFMLSprite());
    }
}

void SFMLWindow::draw(const IText& text)
{
    const auto* sfmlText = dynamic_cast<const SFMLText*>(&text);
    if (sfmlText) {
        window_.draw(sfmlText->getSFMLText());
    }
}

#include <vector>

void SFMLWindow::draw(const Vector2f* vertices, std::size_t vertexCount, Color color, int type)
{
    if (vertexCount == 0 || vertices == nullptr)
        return;

    std::vector<sf::Vertex> sfVertices(vertexCount);
    sf::Color sfColor(color.r, color.g, color.b, color.a);

    for (std::size_t i = 0; i < vertexCount; ++i) {
        sfVertices[i].position = sf::Vector2f(vertices[i].x, vertices[i].y);
        sfVertices[i].color    = sfColor;
    }

    window_.draw(sfVertices.data(), vertexCount, static_cast<sf::PrimitiveType>(type));
}
