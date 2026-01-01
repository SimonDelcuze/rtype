#include "systems/HUDSystem.hpp"

#include "components/ChargeMeterComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/TagComponent.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
    constexpr int kScoreDigits         = 7;
    constexpr float kScoreRightMargin  = 16.0F;
    constexpr float kScoreBottomMargin = 12.0F;
} // namespace

HUDSystem::HUDSystem(Window& window, FontManager& fonts, TextureManager& textureManager)
    : window_(window), fonts_(fonts), textures_(textureManager)
{}

void HUDSystem::updateContent(Registry& registry, EntityId id, TextComponent& textComp) const
{
    if (registry.has<ScoreComponent>(id)) {
        const auto& score = registry.get<ScoreComponent>(id);
        textComp.content  = formatScore(score.value);
    }
}

std::string HUDSystem::formatScore(int value) const
{
    int safeValue      = std::max(0, value);
    std::string digits = std::to_string(safeValue);
    if (static_cast<int>(digits.size()) < kScoreDigits) {
        digits.insert(0, static_cast<std::size_t>(kScoreDigits - static_cast<int>(digits.size())), '0');
    }
    return "SCORE " + digits;
}

void HUDSystem::drawLivesPips(const TransformComponent& transform, const LivesComponent& lives) const
{
    static std::shared_ptr<ISprite> livesSprite = nullptr;
    if (!livesSprite) {
        GraphicsFactory factory;
        livesSprite = factory.createSprite();
        auto tex    = textures_.get("player_ship");
        if (tex) {
            livesSprite->setTexture(*tex);
            livesSprite->setTextureRect({0, 0, 33, 17});
            livesSprite->setScale({1.5f, 1.5f});
        }
    }

    if (!livesSprite)
        return;

    float startX  = transform.x;
    float startY  = transform.y;
    float spacing = 40.0f;

    for (int i = 0; i < lives.current; ++i) {
        livesSprite->setPosition({startX + (static_cast<float>(i) * spacing), startY});
        window_.draw(*livesSprite);
    }
}

void HUDSystem::update(Registry& registry, float)
{
    int playerLives = -1;
    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);
        if (tag.hasTag(EntityTag::Player)) {
            playerLives = registry.get<LivesComponent>(id).current;
            break;
        }
    }

    int playerScore = -1;
    for (EntityId id : registry.view<TagComponent, ScoreComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);
        if (tag.hasTag(EntityTag::Player)) {
            playerScore = registry.get<ScoreComponent>(id).value;
            break;
        }
    }

    for (EntityId id : registry.view<TagComponent, ChargeMeterComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);
        if (tag.hasTag(EntityTag::Player)) {
            float progress = registry.get<ChargeMeterComponent>(id).progress;

            auto size  = window_.getSize();
            float barW = 200.0f;
            float barH = 15.0f;
            float x    = (size.x - barW) / 2.0f;
            float y    = size.y - 40.0f;

            Vector2f bg[4] = {{x, y}, {x, y + barH}, {x + barW, y}, {x + barW, y + barH}};
            window_.draw(bg, 4, Color{100, 100, 100}, 4);

            float fillW    = barW * std::clamp(progress, 0.0f, 1.0f);
            Vector2f fg[4] = {{x, y}, {x, y + barH}, {x + fillW, y}, {x + fillW, y + barH}};
            window_.draw(fg, 4, Color{0, 255, 255}, 4);

            Vector2f outline[5] = {{x, y}, {x + barW, y}, {x + barW, y + barH}, {x, y + barH}, {x, y}};
            window_.draw(outline, 5, Color::White, 2);

            break;
        }
    }

    for (EntityId entity : registry.view<TransformComponent, TextComponent>()) {
        if (!registry.isAlive(entity)) {
            continue;
        }

        auto& transform = registry.get<TransformComponent>(entity);
        auto& textComp  = registry.get<TextComponent>(entity);

        if (registry.has<ScoreComponent>(entity) && playerScore >= 0) {
            registry.get<ScoreComponent>(entity).set(playerScore);
        }

        updateContent(registry, entity, textComp);

        if (!textComp.fontId.empty()) {
            auto font = fonts_.get(textComp.fontId);
            if (font != nullptr) {
                if (!textComp.text) {
                    GraphicsFactory factory;
                    textComp.text = factory.createText();
                    textComp.text->setFont(*font);
                    textComp.text->setCharacterSize(textComp.characterSize);
                }

                textComp.text->setString(textComp.content);
                textComp.text->setFillColor(textComp.color);

                if (registry.has<ScoreComponent>(entity)) {
                    const auto size  = window_.getSize();
                    FloatRect bounds = textComp.text->getLocalBounds();
                    transform.x      = static_cast<float>(size.x) - kScoreRightMargin - (bounds.left + bounds.width);
                    transform.y      = static_cast<float>(size.y) - kScoreBottomMargin - (bounds.top + bounds.height);
                }
                textComp.text->setPosition(Vector2f{transform.x, transform.y});
                textComp.text->setScale(Vector2f{transform.scaleX, transform.scaleY});
                textComp.text->setRotation(transform.rotation);

                window_.draw(*textComp.text);
            }
        }
    }

    for (EntityId id : registry.view<LivesComponent, TransformComponent, LayerComponent>()) {
        if (registry.get<LayerComponent>(id).layer == 100) {
            const auto& transform = registry.get<TransformComponent>(id);
            auto& lives           = registry.get<LivesComponent>(id);

            if (playerLives != -1) {
                lives.current = playerLives;
            }

            drawLivesPips(transform, lives);
        }
    }
}
