#include "systems/LevelEventSystem.hpp"

#include "Logger.hpp"

LevelEventSystem::LevelEventSystem(ThreadSafeQueue<LevelEventData>& queue, const AssetManifest& manifest,
                                   TextureManager& textures)
    : queue_(&queue), manifest_(&manifest), textures_(&textures)
{
    activeScroll_.mode  = LevelScrollMode::Constant;
    activeScroll_.speedX = fallbackSpeed_;
    scrollActive_ = true;
}

void LevelEventSystem::update(Registry& registry, float deltaTime)
{
    LevelEventData event;
    while (queue_->tryPop(event)) {
        applyEvent(registry, event);
    }

    scrollTime_ += deltaTime;
    float speedX = currentScrollSpeed();
    applyScrollSpeed(registry, speedX);
}

void LevelEventSystem::applyEvent(Registry& registry, const LevelEventData& event)
{
    if (event.type == LevelEventType::SetScroll) {
        if (event.scroll.has_value()) {
            applyScrollSettings(*event.scroll);
            applyScrollSpeed(registry, currentScrollSpeed());
        }
        return;
    }
    if (event.type == LevelEventType::SetBackground) {
        if (event.backgroundId.has_value()) {
            applyBackground(registry, *event.backgroundId);
        }
        return;
    }
    if (event.type == LevelEventType::SetMusic) {
        if (event.musicId.has_value()) {
            applyMusic(*event.musicId);
        }
        return;
    }
    if (event.type == LevelEventType::SetCameraBounds) {
        if (event.cameraBounds.has_value()) {
            cameraBounds_ = *event.cameraBounds;
        }
        return;
    }
    if (event.type == LevelEventType::GateOpen) {
        if (event.gateId.has_value()) {
            gateStates_[*event.gateId] = true;
        }
        return;
    }
    if (event.type == LevelEventType::GateClose) {
        if (event.gateId.has_value()) {
            gateStates_[*event.gateId] = false;
        }
        return;
    }
}

void LevelEventSystem::applyBackground(Registry& registry, const std::string& backgroundId)
{
    auto entry = manifest_->findTextureById(backgroundId);
    if (!entry) {
        Logger::instance().warn("[LevelEvent] Unknown background id=" + backgroundId);
        return;
    }
    if (!textures_->has(entry->id)) {
        try {
            textures_->load(entry->id, "client/assets/" + entry->path);
        } catch (...) {
            Logger::instance().warn("[LevelEvent] Failed to load background id=" + entry->id);
            return;
        }
    }
    const sf::Texture* tex = textures_->get(entry->id);
    if (tex == nullptr) {
        return;
    }

    bool updated = false;
    for (EntityId id : registry.view<BackgroundScrollComponent, SpriteComponent>()) {
        if (!registry.isAlive(id)) {
            continue;
        }
        auto& sprite = registry.get<SpriteComponent>(id);
        sprite.setTexture(*tex);
        auto& scroll = registry.get<BackgroundScrollComponent>(id);
        scroll.resetOffsetX = 0.0F;
        scroll.resetOffsetY = 0.0F;
        updated = true;
    }

    if (!updated) {
        EntityId e = registry.createEntity();
        registry.emplace<SpriteComponent>(e, SpriteComponent(*tex));
        registry.emplace<TransformComponent>(e, TransformComponent::create(0.0F, 0.0F));
        registry.emplace<BackgroundScrollComponent>(e,
                                                    BackgroundScrollComponent::create(currentScrollSpeed(), 0.0F, 0.0F));
        registry.emplace<LayerComponent>(e, LayerComponent::create(RenderLayer::Background));
    }
}

void LevelEventSystem::applyScrollSettings(const LevelScrollSettings& settings)
{
    activeScroll_ = settings;
    scrollTime_   = 0.0F;
    scrollActive_ = true;
}

void LevelEventSystem::applyScrollSpeed(Registry& registry, float speedX)
{
    for (EntityId id : registry.view<BackgroundScrollComponent>()) {
        if (!registry.isAlive(id)) {
            continue;
        }
        auto& scroll = registry.get<BackgroundScrollComponent>(id);
        scroll.speedX = speedX;
    }
}

float LevelEventSystem::currentScrollSpeed() const
{
    if (!scrollActive_) {
        return fallbackSpeed_;
    }
    if (activeScroll_.mode == LevelScrollMode::Stopped) {
        return 0.0F;
    }
    if (activeScroll_.mode == LevelScrollMode::Constant) {
        return activeScroll_.speedX;
    }
    if (activeScroll_.curve.empty()) {
        return 0.0F;
    }
    float speed = activeScroll_.curve.front().speedX;
    for (const auto& key : activeScroll_.curve) {
        if (scrollTime_ >= key.time) {
            speed = key.speedX;
        } else {
            break;
        }
    }
    return speed;
}

void LevelEventSystem::applyMusic(const std::string& musicId)
{
    if (currentMusicId_ == musicId) {
        return;
    }
    auto entry = manifest_->findSoundById(musicId);
    if (!entry) {
        Logger::instance().warn("[LevelEvent] Unknown music id=" + musicId);
        return;
    }
    std::string path = "client/assets/" + entry->path;
    if (!music_.openFromFile(path)) {
        Logger::instance().warn("[LevelEvent] Failed to open music path=" + path);
        return;
    }
#if defined(SFML_VERSION_MAJOR) && SFML_VERSION_MAJOR >= 3
    music_.setLooping(true);
#else
    music_.setLoop(true);
#endif
    music_.play();
    currentMusicId_ = musicId;
}
