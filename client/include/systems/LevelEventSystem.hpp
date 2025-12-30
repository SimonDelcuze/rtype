#pragma once

#include "assets/AssetManifest.hpp"
#include "components/BackgroundScrollComponent.hpp"
#include "components/LayerComponent.hpp"
#include "components/SpriteComponent.hpp"
#include "components/TransformComponent.hpp"
#include "concurrency/ThreadSafeQueue.hpp"
#include "ecs/Registry.hpp"
#include "graphics/TextureManager.hpp"
#include "network/LevelEventData.hpp"
#include "systems/ISystem.hpp"

#include <optional>
#include <string>
#include <unordered_map>

class LevelEventSystem : public ISystem
{
  public:
    LevelEventSystem(ThreadSafeQueue<LevelEventData>& queue, const AssetManifest& manifest, TextureManager& textures);

    void update(Registry& registry, float deltaTime) override;

  private:
    void applyEvent(Registry& registry, const LevelEventData& event);
    void applyBackground(Registry& registry, const std::string& backgroundId);
    void applyScrollSettings(const LevelScrollSettings& settings);
    void applyScrollSpeed(Registry& registry, float speedX);
    float currentScrollSpeed() const;

    ThreadSafeQueue<LevelEventData>* queue_ = nullptr;
    const AssetManifest* manifest_          = nullptr;
    TextureManager* textures_               = nullptr;

    LevelScrollSettings activeScroll_;
    float scrollTime_    = 0.0F;
    bool scrollActive_   = false;
    float fallbackSpeed_ = -50.0F;

    std::optional<LevelCameraBounds> cameraBounds_;
    std::unordered_map<std::string, bool> gateStates_;
};
