#include "graphics/backends/sfml/SFMLWindow.hpp"

#include "graphics/backends/sfml/SFMLCommon.hpp"
#include "graphics/backends/sfml/SFMLSprite.hpp"
#include "graphics/backends/sfml/SFMLText.hpp"

#include <array>
#include <iostream>

namespace
{
    const char* colorFilterShaderSrc = R"(
uniform sampler2D texture;
uniform vec3 row0;
uniform vec3 row1;
uniform vec3 row2;

void main()
{
    vec4 color = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;
    vec3 transformed = vec3(dot(row0, color.rgb), dot(row1, color.rgb), dot(row2, color.rgb));
    gl_FragColor = vec4(clamp(transformed, 0.0, 1.0), color.a);
}
)";

    std::array<float, 9> matrixForMode(ColorFilterMode mode)
    {
        switch (mode) {
            case ColorFilterMode::Protanopia:
                return {0.56667F, 0.43333F, 0.0F, 0.55833F, 0.44167F, 0.0F, 0.0F, 0.24167F, 0.75833F};
            case ColorFilterMode::Deuteranopia:
                return {0.625F, 0.375F, 0.0F, 0.7F, 0.3F, 0.0F, 0.0F, 0.3F, 0.7F};
            case ColorFilterMode::Tritanopia:
                return {0.95F, 0.05F, 0.0F, 0.0F, 0.43333F, 0.56667F, 0.0F, 0.475F, 0.525F};
            case ColorFilterMode::Achromatopsia:
                return {0.299F, 0.587F, 0.114F, 0.299F, 0.587F, 0.114F, 0.299F, 0.587F, 0.114F};
            case ColorFilterMode::None:
            default:
                return {1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F};
        }
    }

    void applyMatrix(sf::Shader& shader, ColorFilterMode mode)
    {
        auto m = matrixForMode(mode);
        shader.setUniform("row0", sf::Glsl::Vec3(m[0], m[1], m[2]));
        shader.setUniform("row1", sf::Glsl::Vec3(m[3], m[4], m[5]));
        shader.setUniform("row2", sf::Glsl::Vec3(m[6], m[7], m[8]));
    }
} // namespace

SFMLWindow::SFMLWindow(unsigned int width, unsigned int height, const std::string& title)
    : window_(sf::VideoMode({width, height}), title)
{
    renderTextureReady_ = renderTexture_.resize({width, height});
    shaderReady_ =
        sf::Shader::isAvailable() && colorShader_.loadFromMemory(colorFilterShaderSrc, sf::Shader::Type::Fragment);
    if (shaderReady_) {
        colorShader_.setUniform("texture", sf::Shader::CurrentTexture);
        applyMatrix(colorShader_, colorFilterMode_);
    }
}

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
    activeTarget().clear(toSFML(color));
}

void SFMLWindow::display()
{
    if (!useColorFilter()) {
        window_.display();
        return;
    }

    renderTexture_.display();
    window_.clear();
    sf::Sprite sprite(renderTexture_.getTexture());
    sf::RenderStates states;
    states.shader = &colorShader_;
    window_.draw(sprite, states);
    window_.display();
}

void SFMLWindow::draw(const ISprite& sprite)
{
    const auto* sfmlSprite = dynamic_cast<const SFMLSprite*>(&sprite);
    if (sfmlSprite) {
        activeTarget().draw(sfmlSprite->getSFMLSprite());
    }
}

void SFMLWindow::draw(const IText& text)
{
    const auto* sfmlText = dynamic_cast<const SFMLText*>(&text);
    if (sfmlText) {
        activeTarget().draw(sfmlText->getSFMLText());
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

    activeTarget().draw(sfVertices.data(), vertexCount, static_cast<sf::PrimitiveType>(type));
}

void SFMLWindow::drawRectangle(Vector2f size, Vector2f position, float rotation, Vector2f scale, Color fillColor,
                               Color outlineColor, float outlineThickness)
{
    sf::RectangleShape rect(sf::Vector2f(size.x, size.y));
    rect.setPosition(sf::Vector2f(position.x, position.y));
    rect.setRotation(sf::degrees(rotation));
    rect.setScale(sf::Vector2f(scale.x, scale.y));
    rect.setFillColor(sf::Color(fillColor.r, fillColor.g, fillColor.b, fillColor.a));
    rect.setOutlineColor(sf::Color(outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a));
    rect.setOutlineThickness(outlineThickness);
    activeTarget().draw(rect);
}

void SFMLWindow::setColorFilter(ColorFilterMode mode)
{
    if (colorFilterMode_ == mode)
        return;
    colorFilterMode_ = mode;
    if (shaderReady_) {
        applyMatrix(colorShader_, colorFilterMode_);
    }
}

ColorFilterMode SFMLWindow::getColorFilter() const
{
    return colorFilterMode_;
}

bool SFMLWindow::useColorFilter() const
{
    return colorFilterMode_ != ColorFilterMode::None && shaderReady_ && renderTextureReady_;
}

sf::RenderTarget& SFMLWindow::activeTarget()
{
    if (useColorFilter())
        return renderTexture_;
    return window_;
}
