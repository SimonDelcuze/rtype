#pragma once

#include "audio/SoundManager.hpp"
#include "components/AudioComponent.hpp"
#include "ecs/Registry.hpp"
#include "graphics/GraphicsFactory.hpp"
#include "graphics/abstraction/ISound.hpp"
#include "systems/ISystem.hpp"

#include <memory>
#include <unordered_map>

class AudioSystem : public ISystem
{
  public:
    AudioSystem(SoundManager& soundManager, GraphicsFactory& graphicsFactory);

    void update(Registry& registry, float deltaTime) override;

  private:
    SoundManager& soundManager_;
    GraphicsFactory& graphicsFactory_;
    std::unordered_map<EntityId, std::unique_ptr<ISound>> sounds_;
};
