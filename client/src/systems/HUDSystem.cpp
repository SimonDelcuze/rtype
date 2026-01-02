#include "systems/HUDSystem.hpp"

#include "components/BossComponent.hpp"
#include "components/ChargeMeterComponent.hpp"
#include "components/HealthComponent.hpp"
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
            livesSprite->setScale({1.0f, 1.0f});
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
    const BossComponent* boss         = nullptr;
    const HealthComponent* bossHealth = nullptr;
    for (EntityId id : registry.view<BossComponent, HealthComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& health = registry.get<HealthComponent>(id);
        if (health.max <= 0 || health.current <= 0)
            continue;
        boss       = &registry.get<BossComponent>(id);
        bossHealth = &health;
        break;
    }
    if (boss != nullptr && bossHealth != nullptr) {
        const auto size       = window_.getSize();
        const float barWidth  = std::min(640.0F, static_cast<float>(size.x) * 0.7F);
        const float barHeight = 16.0F;
        const float x         = (static_cast<float>(size.x) - barWidth) / 2.0F;
        const float y         = 20.0F;

        float maxHealth = static_cast<float>(std::max(1, bossHealth->max));
        float ratio     = static_cast<float>(bossHealth->current) / maxHealth;
        ratio           = std::clamp(ratio, 0.0F, 1.0F);

        Vector2f bg[4] = {{x, y}, {x, y + barHeight}, {x + barWidth, y}, {x + barWidth, y + barHeight}};
        window_.draw(bg, 4, Color{30, 20, 20, 200}, 4);

        Vector2f fg[4] = {
            {x, y}, {x, y + barHeight}, {x + (barWidth * ratio), y}, {x + (barWidth * ratio), y + barHeight}};
        window_.draw(fg, 4, Color{220, 60, 60, 230}, 4);

        Vector2f outline[5] = {{x, y}, {x + barWidth, y}, {x + barWidth, y + barHeight}, {x, y + barHeight}, {x, y}};
        window_.draw(outline, 5, Color{140, 60, 60, 220}, 2);

        auto font = fonts_.get("score_font");
        if (font != nullptr) {
            static std::shared_ptr<IText> bossText = nullptr;
            if (!bossText) {
                GraphicsFactory factory;
                bossText = factory.createText();
            }
            if (bossText) {
                bossText->setFont(*font);
                bossText->setCharacterSize(14);
                bossText->setString(boss->name.empty() ? "BOSS" : boss->name);
                bossText->setFillColor(Color{240, 220, 220});
                bossText->setPosition(Vector2f{x, y - 18.0F});
                bossText->setScale(Vector2f{1.0F, 1.0F});
                bossText->setRotation(0.0F);
                window_.draw(*bossText);
            }
        }
    }
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

    float chargeProgress = 0.0f;
    bool hasCharge       = false;
    for (EntityId e : registry.view<ChargeMeterComponent>()) {
        if (!registry.isAlive(e))
            continue;
        chargeProgress = std::clamp(registry.get<ChargeMeterComponent>(e).progress, 0.0f, 1.0f);
        hasCharge      = true;
        break;
    }

    if (hasCharge) {
        auto size       = window_.getSize();
        float barWidth  = 220.0f;
        float barHeight = 12.0f;
        float x         = (static_cast<float>(size.x) - barWidth) / 2.0f;
        float y         = static_cast<float>(size.y) - 30.0f;

        window_.drawRectangle({barWidth, barHeight}, {x, y}, 0.0f, {1.0f, 1.0f}, Color{20, 20, 40, 160},
                              Color{80, 120, 220, 200}, 2.0f);

        if (chargeProgress > 0.0f) {
            float fillWidth = barWidth * chargeProgress;
            window_.drawRectangle({fillWidth, barHeight}, {x, y}, 0.0f, {1.0f, 1.0f}, Color{70, 160, 255, 230},
                                  Color::Transparent, 0.0f);
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
