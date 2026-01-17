#include "systems/HUDSystem.hpp"

#include "components/AllyComponent.hpp"
#include "components/BossComponent.hpp"
#include "components/ChargeMeterComponent.hpp"
#include "components/HealthComponent.hpp"
#include "components/OwnershipComponent.hpp"
#include "components/RenderTypeComponent.hpp"
#include "components/ShieldComponent.hpp"
#include "components/TagComponent.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/Common.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr int kScoreDigits                  = 7;
    constexpr float kScoreRightMargin           = 16.0F;
    constexpr float kScoreBottomMargin          = 12.0F;
    constexpr float kLivesLeftMargin            = 16.0F;
    constexpr float kLivesBottomMargin          = 12.0F;
    constexpr float kLivesRowSpacing            = 26.0F;
    constexpr float kLivesPipSpacing            = 40.0F;
    constexpr float kLivesPipHeight             = 17.0F;
    constexpr float kLivesChargeOffset          = 24.0F;
    constexpr std::uint16_t kShieldRenderTypeId = 25;
} // namespace

HUDSystem::HUDSystem(Window& window, FontManager& fonts, TextureManager& textureManager, std::uint32_t localPlayerId,
                     RoomType gameMode)
    : window_(window), fonts_(fonts), textures_(textureManager), state_(nullptr), localPlayerId_(localPlayerId),
      gameMode_(gameMode)
{}

HUDSystem::HUDSystem(Window& window, FontManager& fonts, TextureManager& textureManager, LevelState& state,
                     std::uint32_t localPlayerId, RoomType gameMode)
    : window_(window), fonts_(fonts), textures_(textureManager), state_(&state), localPlayerId_(localPlayerId),
      gameMode_(gameMode)
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

void HUDSystem::drawLivesPips(float startX, float startY, int lives, const std::shared_ptr<ITexture>& tex,
                              const IntRect& rect) const
{
    if (!tex || lives <= 0)
        return;

    GraphicsFactory factory;
    auto sprite = factory.createSprite();
    sprite->setTexture(*tex);
    sprite->setTextureRect(rect);
    sprite->setScale({1.0f, 1.0f});

    for (int i = 0; i < lives; ++i) {
        sprite->setPosition({startX + (static_cast<float>(i) * kLivesPipSpacing), startY});
        window_.draw(*sprite);
    }
}

void HUDSystem::update(Registry& registry, float deltaTime)
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
    const bool bossActive = (boss != nullptr && bossHealth != nullptr);
    if (bossActive) {
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

    if (state_ != nullptr && state_->safeZoneActive && !bossActive) {
        auto font = fonts_.get("score_font");
        if (font != nullptr) {
            static std::shared_ptr<IText> safeZoneText = nullptr;
            if (!safeZoneText) {
                GraphicsFactory factory;
                safeZoneText = factory.createText();
            }
            if (safeZoneText) {
                safeZoneText->setFont(*font);
                safeZoneText->setCharacterSize(22);
                safeZoneText->setString("SAFE ZONE, PRESS FIRE TO CONTINUE");
                safeZoneText->setFillColor(Color{240, 240, 240});
                const auto size  = window_.getSize();
                FloatRect bounds = safeZoneText->getLocalBounds();
                safeZoneText->setOrigin(Vector2f{bounds.left + bounds.width / 2.0F, bounds.top + bounds.height / 2.0F});
                float y = 24.0F;
                safeZoneText->setPosition(Vector2f{static_cast<float>(size.x) / 2.0F, y});
                safeZoneText->setScale(Vector2f{1.0F, 1.0F});
                safeZoneText->setRotation(0.0F);
                window_.draw(*safeZoneText);
            }
        }
    }
    if (state_ != nullptr && state_->shieldFeedbackTimeRemaining > 0.0F) {
        state_->shieldFeedbackTimeRemaining =
            std::max(0.0F, state_->shieldFeedbackTimeRemaining - std::max(0.0F, deltaTime));
    }
    int playerScore = 0;

    for (EntityId id : registry.view<ScoreComponent, OwnershipComponent>()) {
        const auto& owner = registry.get<OwnershipComponent>(id);
        if (owner.ownerId == localPlayerId_) {
            playerScore = registry.get<ScoreComponent>(id).value;
            break;
        }
    }
    bool hasShield = false;
    for (EntityId id : registry.view<RenderTypeComponent, OwnershipComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& renderType = registry.get<RenderTypeComponent>(id);
        if (renderType.typeId != kShieldRenderTypeId)
            continue;
        const auto& owner = registry.get<OwnershipComponent>(id);
        if (owner.ownerId == localPlayerId_) {
            hasShield = true;
            break;
        }
    }

	    auto font = fonts_.get("score_font");
	    if (font != nullptr && state_ != nullptr && state_->safeZoneActive && !bossActive &&
	        playerScore >= AllyComponent::kAllyCost) {
        static std::shared_ptr<IText> allyText = nullptr;
        if (!allyText) {
            GraphicsFactory factory;
            allyText = factory.createText();
        }
	        if (allyText) {
	            allyText->setFont(*font);
	            allyText->setCharacterSize(18);
	            allyText->setString("Press E to buy Ally (10000 pts)");
	            allyText->setFillColor(Color{200, 255, 200});
	            const auto screenSize = window_.getSize();
	            FloatRect allyBounds  = allyText->getLocalBounds();
            allyText->setOrigin(
                Vector2f{allyBounds.left + allyBounds.width / 2.0F, allyBounds.top + allyBounds.height / 2.0F});
            float allyY = bossActive ? 76.0F : 52.0F;
            allyText->setPosition(Vector2f{static_cast<float>(screenSize.x) / 2.0F, allyY});
            allyText->setScale(Vector2f{1.0F, 1.0F});
            allyText->setRotation(0.0F);
            window_.draw(*allyText);
        }
	    }

	    if (font != nullptr && state_ != nullptr && state_->safeZoneActive && !bossActive &&
	        playerScore >= ShieldComponent::kShieldCost && !hasShield) {
	        static std::shared_ptr<IText> shieldText = nullptr;
	        if (!shieldText) {
	            GraphicsFactory factory;
	            shieldText = factory.createText();
	        }
	        if (shieldText) {
	            shieldText->setFont(*font);
	            shieldText->setCharacterSize(18);
	            shieldText->setString("Press F to buy Shield (5000 pts)");
	            shieldText->setFillColor(Color{200, 255, 200});
	            const auto screenSize  = window_.getSize();
	            FloatRect shieldBounds = shieldText->getLocalBounds();
            shieldText->setOrigin(
                Vector2f{shieldBounds.left + shieldBounds.width / 2.0F, shieldBounds.top + shieldBounds.height / 2.0F});
            float shieldY = bossActive ? 100.0F : 76.0F;
            shieldText->setPosition(Vector2f{static_cast<float>(screenSize.x) / 2.0F, shieldY});
            shieldText->setScale(Vector2f{1.0F, 1.0F});
            shieldText->setRotation(0.0F);
            window_.draw(*shieldText);
        }
    }
    if (font != nullptr && hasShield) {
        static std::shared_ptr<IText> shieldActiveText = nullptr;
        if (!shieldActiveText) {
            GraphicsFactory factory;
            shieldActiveText = factory.createText();
        }
        if (shieldActiveText) {
            shieldActiveText->setFont(*font);
            shieldActiveText->setCharacterSize(18);
            shieldActiveText->setString("Shield active");
            shieldActiveText->setFillColor(Color{200, 220, 255});
            const auto screenSize  = window_.getSize();
            FloatRect shieldBounds = shieldActiveText->getLocalBounds();
            float shieldY          = bossActive ? 100.0F : 76.0F;
            shieldActiveText->setOrigin(
                Vector2f{shieldBounds.left + shieldBounds.width / 2.0F, shieldBounds.top + shieldBounds.height / 2.0F});
            shieldActiveText->setPosition(Vector2f{static_cast<float>(screenSize.x) / 2.0F, shieldY});
            shieldActiveText->setScale(Vector2f{1.0F, 1.0F});
            shieldActiveText->setRotation(0.0F);
            window_.draw(*shieldActiveText);
        }
    }
    if (font != nullptr && state_ != nullptr && state_->shieldFeedbackTimeRemaining > 0.0F) {
        std::string feedbackText;
        if (state_->shieldFeedback == LevelState::ShieldFeedback::AlreadyActive) {
            feedbackText = "Shield already active";
        } else if (state_->shieldFeedback == LevelState::ShieldFeedback::PurchaseRequested) {
            feedbackText = "Shield purchase requested";
        }
        if (!feedbackText.empty()) {
            static std::shared_ptr<IText> shieldFeedbackText = nullptr;
            if (!shieldFeedbackText) {
                GraphicsFactory factory;
                shieldFeedbackText = factory.createText();
            }
            if (shieldFeedbackText) {
                shieldFeedbackText->setFont(*font);
                shieldFeedbackText->setCharacterSize(16);
                shieldFeedbackText->setString(feedbackText);
                shieldFeedbackText->setFillColor(Color{255, 230, 160});
                const auto screenSize = window_.getSize();
                FloatRect bounds      = shieldFeedbackText->getLocalBounds();
                float feedbackY       = bossActive ? 124.0F : 100.0F;
                shieldFeedbackText->setOrigin(
                    Vector2f{bounds.left + bounds.width / 2.0F, bounds.top + bounds.height / 2.0F});
                shieldFeedbackText->setPosition(Vector2f{static_cast<float>(screenSize.x) / 2.0F, feedbackY});
                shieldFeedbackText->setScale(Vector2f{1.0F, 1.0F});
                shieldFeedbackText->setRotation(0.0F);
                window_.draw(*shieldFeedbackText);
            }
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

        if (registry.has<ScoreComponent>(entity)) {
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
                } else if (textComp.centered) {
                    const auto size  = window_.getSize();
                    FloatRect bounds = textComp.text->getLocalBounds();
                    textComp.text->setOrigin(
                        Vector2f{bounds.left + bounds.width / 2.0F, bounds.top + bounds.height / 2.0F});
                    transform.x = static_cast<float>(size.x) / 2.0F + textComp.centerOffsetX;
                    transform.y = static_cast<float>(size.y) / 2.0F + textComp.centerOffsetY;
                } else {
                    textComp.text->setOrigin(Vector2f{0.0F, 0.0F});
                }
                textComp.text->setPosition(Vector2f{transform.x, transform.y});
                textComp.text->setScale(Vector2f{transform.scaleX, transform.scaleY});
                textComp.text->setRotation(transform.rotation);

                window_.draw(*textComp.text);
            }
        }
    }

    struct LivesDisplay
    {
        std::uint32_t owner{0};
        int lives{0};
        std::shared_ptr<ITexture> texture;
        IntRect rect{0, 0, 34, 18};
    };

    std::vector<LivesDisplay> playerLives;
    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        if (!registry.isAlive(id))
            continue;
        const auto& tag = registry.get<TagComponent>(id);
        if (!tag.hasTag(EntityTag::Player))
            continue;
        std::uint32_t key = id;
        if (registry.has<OwnershipComponent>(id)) {
            key = registry.get<OwnershipComponent>(id).ownerId;
        }

        LivesDisplay disp;
        disp.owner = key;
        disp.lives = registry.get<LivesComponent>(id).current;

        if (textures_.has("player_ship")) {
            disp.texture = textures_.get("player_ship");
        }

        int rowIndex = 0;
        if (registry.has<RenderTypeComponent>(id)) {
            auto typeId = registry.get<RenderTypeComponent>(id).typeId;
            if (typeId == 1) {
                rowIndex = 0;
            } else if (typeId == 12) {
                rowIndex = 1;
            } else if (typeId == 13) {
                rowIndex = 2;
            } else {
                rowIndex = 3;
            }
        }
        disp.rect = IntRect{66, rowIndex * 18, 34, 18};

        playerLives.push_back(std::move(disp));
    }
    std::sort(playerLives.begin(), playerLives.end(),
              [](const LivesDisplay& a, const LivesDisplay& b) { return a.owner < b.owner; });

    if (!playerLives.empty()) {
        const auto size = window_.getSize();
        float baseY     = static_cast<float>(size.y) - kLivesBottomMargin;
        if (hasCharge) {
            baseY -= kLivesChargeOffset;
        }
        for (std::size_t i = 0; i < playerLives.size(); ++i) {
            int livesCount = std::max(0, playerLives[i].lives);
            float startX   = kLivesLeftMargin;
            float startY   = baseY - kLivesPipHeight - (static_cast<float>(i) * kLivesRowSpacing);
            drawLivesPips(startX, startY, livesCount, playerLives[i].texture, playerLives[i].rect);
        }
    }
}
