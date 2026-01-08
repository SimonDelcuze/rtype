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
    text->setCharacterSize(20);

    const auto windowSize     = window_.getSize();
    float currentY            = 30.0F;
    const float padding       = 15.0F;
    const float animDuration  = 0.3F;
    const float totalDuration = 5.0F;

    for (const auto& notif : notifications_) {
        float alpha   = 255.0F;
        float xOffset = 0.0F;

        if (notif.timer > totalDuration - animDuration) {
            float t = (totalDuration - notif.timer) / animDuration;
            alpha   = t * 255.0F;
            xOffset = (1.0F - t) * 50.0F;
        } else if (notif.timer < animDuration) {
            float t = notif.timer / animDuration;
            alpha   = t * 255.0F;
            xOffset = (1.0F - t) * 50.0F;
        }

        std::uint8_t uAlpha  = static_cast<std::uint8_t>(std::clamp(alpha, 0.0F, 255.0F));
        std::uint8_t bgAlpha = static_cast<std::uint8_t>(std::clamp(alpha * 0.8F, 0.0F, 200.0F));

        text->setString(notif.message);
        text->setFillColor(Color(255, 80, 80, uAlpha));
        text->setCharacterSize(24);

        FloatRect bounds = text->getLocalBounds();
        float width      = std::max(250.0F, bounds.width + (padding * 2.0F));
        float height     = 45.0F;
        float x          = static_cast<float>(windowSize.x) - width - 20.0F + xOffset;

        window_.drawRectangle({width, height}, {x, currentY}, 0.0F, {1.0F, 1.0F}, Color{20, 20, 25, bgAlpha},
                              Color{100, 100, 100, static_cast<std::uint8_t>(bgAlpha * 0.75F)}, 1.0F);

        text->setOrigin({0.0F, 0.0F});
        text->setPosition({x + padding, currentY + (height / 2.0F) - (bounds.height / 2.0F) - bounds.top});
        window_.draw(*text);

        currentY += height + 10.0F;
    }
}
