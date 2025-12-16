#include "systems/HUDSystem.hpp"

#include "components/ChargeMeterComponent.hpp"
#include "components/TagComponent.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <string>

HUDSystem::HUDSystem(Window& window, FontManager& fonts, TextureManager& textures)
    : window_(window), fonts_(fonts), textures_(textures)
{}

void HUDSystem::updateContent(Registry& registry, EntityId id, TextComponent& textComp) const
{
    if (registry.has<ScoreComponent>(id)) {
        const auto& score = registry.get<ScoreComponent>(id);
        textComp.content  = "Score: " + std::to_string(score.value);
    } else if (registry.has<LivesComponent>(id)) {
        textComp.content = "Lives:";
    }
}

void HUDSystem::drawLivesPips(const TransformComponent& transform, const LivesComponent& lives) const
{
    const sf::Texture* tex = nullptr;
    try {
        if (textures_.has("player_ship")) {
            tex = textures_.get("player_ship");
        }
    } catch (...) {
    }

    if (tex == nullptr) {
        const float pipSize    = 10.0F;
        const float pipSpacing = 4.0F;
        sf::RectangleShape pip(sf::Vector2f{pipSize, pipSize});
        pip.setFillColor(sf::Color(220, 60, 60));

        float startX = transform.x;
        float y      = transform.y + 24.0F + 6.0F;

        int count = std::max(0, lives.current);
        for (int i = 0; i < count; ++i) {
            pip.setPosition(sf::Vector2f{startX + ((pipSize + pipSpacing) * static_cast<float>(i)), y});
            window_.draw(pip);
        }
        return;
    }

    sf::Sprite sprite(*tex);

    int frameW = static_cast<int>(tex->getSize().x) / 5;
    if (frameW <= 0)
        frameW = 33;
    int frameH = 14;

    sprite.setTextureRect(sf::IntRect({0, 0}, {frameW, frameH}));
    sprite.setColor(sf::Color(255, 255, 255, 200));
    sprite.setScale(sf::Vector2f{0.5F, 0.5F});

    float startX = transform.x;
    float y      = transform.y + 24.0F;

    int count = std::max(0, lives.current);
    for (int i = 0; i < count; ++i) {
        sprite.setPosition(
            sf::Vector2f{startX + (((static_cast<float>(frameW) * 0.5F) + 5.0F) * static_cast<float>(i)), y});
        window_.draw(sprite);
    }
}

void HUDSystem::update(Registry& registry, float)
{
    float chargeProgress = 0.0F;
    bool hasCharge       = false;
    for (EntityId e : registry.view<ChargeMeterComponent>()) {
        if (!registry.isAlive(e))
            continue;
        chargeProgress = std::clamp(registry.get<ChargeMeterComponent>(e).progress, 0.0F, 1.0F);
        hasCharge      = true;
        break;
    }

    if (hasCharge) {
        const auto size       = window_.raw().getSize();
        const float barWidth  = 220.0F;
        const float barHeight = 12.0F;
        const float x         = (static_cast<float>(size.x) - barWidth) / 2.0F;
        const float y         = static_cast<float>(size.y) - 30.0F;

        sf::RectangleShape background(sf::Vector2f{barWidth, barHeight});
        background.setPosition(sf::Vector2f{x, y});
        background.setFillColor(sf::Color(20, 20, 40, 160));
        background.setOutlineThickness(2.0F);
        background.setOutlineColor(sf::Color(80, 120, 220, 200));

        sf::RectangleShape fill(sf::Vector2f{barWidth * chargeProgress, barHeight});
        fill.setPosition(sf::Vector2f{x, y});
        fill.setFillColor(sf::Color(70, 160, 255, 230));

        window_.draw(background);
        window_.draw(fill);
    }

    int playerLives = -1;
    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);
        if (tag.hasTag(EntityTag::Player)) {
            playerLives = registry.get<LivesComponent>(id).current;
            break;
        }
    }

    for (EntityId entity : registry.view<TransformComponent, TextComponent>()) {
        if (!registry.isAlive(entity)) {
            continue;
        }

        auto& transform = registry.get<TransformComponent>(entity);
        auto& textComp  = registry.get<TextComponent>(entity);

        updateContent(registry, entity, textComp);

        if (!textComp.fontId.empty()) {
            const sf::Font* font = fonts_.get(textComp.fontId);
            if (font != nullptr) {
                if (!textComp.text.has_value()) {
                    textComp.text.emplace(*font, textComp.content, textComp.characterSize);
                } else {
                    textComp.text->setFont(*font);
                    textComp.text->setCharacterSize(textComp.characterSize);
                    textComp.text->setString(textComp.content);
                }

                textComp.text->setFillColor(textComp.color);
                textComp.text->setPosition(sf::Vector2f{transform.x, transform.y});
                textComp.text->setScale(sf::Vector2f{transform.scaleX, transform.scaleY});
                textComp.text->setRotation(sf::degrees(transform.rotation));

                window_.draw(*textComp.text);
            }
        }

        if (registry.has<LivesComponent>(entity)) {
            auto& lives = registry.get<LivesComponent>(entity);
            if (playerLives != -1) {
                lives.current = playerLives;
            }
            drawLivesPips(transform, lives);
        }
    }
}
