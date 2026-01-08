#include "systems/NotificationSystem.hpp"

#include "Logger.hpp"
#include "graphics/GraphicsFactory.hpp"

#include <algorithm>

NotificationSystem::NotificationSystem(Window& window, FontManager& fonts, ThreadSafeQueue<std::string>& broadcastQueue)
    : window_(window), fonts_(fonts), broadcastQueue_(broadcastQueue)
{}

NotificationSystem::~NotificationSystem()
{
    for (const auto& notif : notifications_) {
        broadcastQueue_.push(notif.message);
    }
}

void NotificationSystem::update(Registry&, float deltaTime)
{
    std::string newMsg;
    while (broadcastQueue_.tryPop(newMsg)) {
        Logger::instance().info("[NotificationSystem] Received message: " + newMsg);
        notifications_.push_back({newMsg, 5.0F});
    }

    for (auto it = notifications_.begin(); it != notifications_.end();) {
        it->timer -= deltaTime;
        if (it->timer <= 0.0F) {
            it = notifications_.erase(it);
        } else {
            ++it;
        }
    }

    if (notifications_.empty()) {
        return;
    }
    auto font = fonts_.get("ui");
    if (!font) {
        font = fonts_.get("score_font");
    }
    if (!font) {
        Logger::instance().error("[NotificationSystem] Font 'ui' or 'score_font' not found!");
        return;
    }

    GraphicsFactory factory;
    auto text = factory.createText();
    text->setFont(*font);

    const auto windowSize     = window_.getSize();
    float currentY            = 50.0F;
    const float padding       = 25.0F;
    const float animDuration  = 0.4F;
    const float totalDuration = 5.0F;

    for (const auto& notif : notifications_) {
        float alpha   = 255.0F;
        float scale   = 1.0F;
        float yOffset = 0.0F;

        if (notif.timer > totalDuration - animDuration) {
            float t = (totalDuration - notif.timer) / animDuration;
            alpha   = t * 255.0F;
            scale   = 0.8F + (t * 0.2F);
            yOffset = (1.0F - t) * -20.0F;
        } else if (notif.timer < animDuration) {
            float t = notif.timer / animDuration;
            alpha   = t * 255.0F;
            scale   = 0.8F + (t * 0.2F);
            yOffset = (1.0F - t) * -20.0F;
        }

        std::uint8_t uAlpha  = static_cast<std::uint8_t>(std::clamp(alpha, 0.0F, 255.0F));
        std::uint8_t bgAlpha = static_cast<std::uint8_t>(std::clamp(alpha * 0.85F, 0.0F, 220.0F));

        text->setString(notif.message);
        text->setCharacterSize(40);
        text->setFillColor(Color(255, 100, 100, uAlpha));
        text->setOutlineColor(Color(0, 0, 0, uAlpha));
        text->setOutlineThickness(2.0F);

        FloatRect bounds = text->getLocalBounds();
        float width      = bounds.width + (padding * 2.0F);
        float height     = bounds.height + (padding * 1.5F);
        float x          = (static_cast<float>(windowSize.x) / 2.0F) - (width / 2.0F);
        float y          = currentY + yOffset;

        window_.drawRectangle({width, height}, {x, y}, 0.0F, {scale, scale}, Color{30, 30, 35, bgAlpha},
                              Color{150, 50, 50, static_cast<std::uint8_t>(bgAlpha * 0.8F)}, 2.0F);

        text->setOrigin({bounds.left + bounds.width / 2.0F, bounds.top + bounds.height / 2.0F});
        text->setPosition({x + width / 2.0F, y + height / 2.0F});
        text->setScale({scale, scale});
        window_.draw(*text);

        currentY += (height * scale) + 15.0F;
    }
}
