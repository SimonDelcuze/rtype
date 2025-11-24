#pragma once

#include "audio/SoundManager.hpp"
#include "components/AudioComponent.hpp"
#include "ecs/Registry.hpp"

#include <SFML/Audio/Sound.hpp>
#include <memory>
#include <unordered_map>

class AudioSystem
{
  public:
    explicit AudioSystem(SoundManager& soundManager);

    void update(Registry& registry);

  private:
    SoundManager& soundManager_;
    std::unordered_map<EntityId, std::unique_ptr<sf::Sound>> sounds_;
};
