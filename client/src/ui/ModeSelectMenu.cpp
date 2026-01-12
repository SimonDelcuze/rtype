#include "ui/ModeSelectMenu.hpp"

#include "Logger.hpp"
#include "components/BoxComponent.hpp"
#include "components/ButtonComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TextComponent.hpp"
#include "components/TransformComponent.hpp"

namespace
{
    EntityId createBackground(Registry& registry, TextureManager& textures)
    {
        if (!textures.has("menu_bg"))
            textures.load("menu_bg", "client/assets/backgrounds/menu.jpg");

        auto tex = textures.get("menu_bg");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 0.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.25F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createLogo(Registry& registry, TextureManager& textures)
    {
        if (!textures.has("logo"))
            textures.load("logo", "client/assets/other/rtype-logo.png");

        auto tex = textures.get("logo");
        if (tex == nullptr)
            return 0;

        EntityId entity  = registry.createEntity();
        auto& transform  = registry.emplace<TransformComponent>(entity);
        transform.x      = 325.0F;
        transform.y      = 0.0F;
        transform.scaleX = 2.0F;
        transform.scaleY = 2.0F;

        SpriteComponent sprite(tex);
        registry.emplace<SpriteComponent>(entity, sprite);
        return entity;
    }

    EntityId createButton(Registry& registry, float x, float y, float w, float h, const std::string& label,
                          const Color& fill, std::function<void()> onClick)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto box =
            BoxComponent::create(w, h, fill,
                                 Color(static_cast<std::uint8_t>(fill.r + 40), static_cast<std::uint8_t>(fill.g + 40),
                                       static_cast<std::uint8_t>(fill.b + 40)));
        box.focusColor = Color(100, 200, 255);
        registry.emplace<BoxComponent>(entity, box);
        registry.emplace<ButtonComponent>(entity, ButtonComponent::create(label, onClick));
        return entity;
    }

    EntityId createText(Registry& registry, float x, float y, const std::string& content, unsigned int size,
                        const Color& color)
    {
        EntityId entity = registry.createEntity();
        auto& transform = registry.emplace<TransformComponent>(entity);
        transform.x     = x;
        transform.y     = y;

        auto text    = TextComponent::create("ui", size, color);
        text.content = content;
        registry.emplace<TextComponent>(entity, text);
        return entity;
    }
} // namespace

ModeSelectMenu::ModeSelectMenu(FontManager& fonts, TextureManager& textures) : fonts_(fonts), textures_(textures) {}

void ModeSelectMenu::create(Registry& registry)
{
    done_       = false;
    result_     = Result{};
    background_ = createBackground(registry, textures_);
    logo_       = createLogo(registry, textures_);
    title_      = createText(registry, 420.0F, 220.0F, "Choose Mode", 36, Color::White);
    quickBtn_   = createButton(registry, 430.0F, 320.0F, 220.0F, 60.0F, "Quickplay", Color(0, 120, 200), [this]() {
        result_.selected  = RoomType::Quickplay;
        result_.confirmed = true;
        done_             = true;
        Logger::instance().info("[ModeSelect] Quickplay selected");
    });
    rankedBtn_  = createButton(registry, 680.0F, 320.0F, 220.0F, 60.0F, "Ranked", Color(0, 80, 160), [this]() {
        result_.selected  = RoomType::Ranked;
        result_.confirmed = true;
        done_             = true;
        Logger::instance().info("[ModeSelect] Ranked selected");
    });
    backBtn_    = createButton(registry, 560.0F, 420.0F, 200.0F, 50.0F, "Back", Color(120, 50, 50), [this]() {
        result_.backRequested = true;
        done_                 = true;
        Logger::instance().info("[ModeSelect] Back requested");
    });
}

void ModeSelectMenu::destroy(Registry& registry)
{
    for (EntityId id : {background_, logo_, title_, quickBtn_, rankedBtn_, backBtn_}) {
        if (registry.isAlive(id))
            registry.destroyEntity(id);
    }
}

bool ModeSelectMenu::isDone() const
{
    return done_;
}

void ModeSelectMenu::handleEvent(Registry&, const Event&) {}

void ModeSelectMenu::update(Registry&, float) {}

void ModeSelectMenu::render(Registry&, Window&) {}
