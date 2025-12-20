#include "systems/GameOverSystem.hpp"

#include "Logger.hpp"
#include "components/LivesComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TagComponent.hpp"
#include "ecs/Registry.hpp"
#include "events/GameEvents.hpp"

GameOverSystem::GameOverSystem(EventBus& eventBus) : eventBus_(eventBus)
{
    Logger::instance().info("[GameOverSystem] Initialized");
}

void GameOverSystem::update(Registry& registry, float deltaTime)
{
    (void) deltaTime;

    if (gameOverTriggered_) {
        return;
    }

    std::vector<EntityId> playerIds;

    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);

        if (!tag.hasTag(EntityTag::Player)) {
            continue;
        }

        playerIds.push_back(id);
        const auto& lives = registry.get<LivesComponent>(id);

        if (lives.isDead()) {
            GameOverEvent event{};
            event.victory    = false;
            event.finalScore = 0;
            if (registry.has<ScoreComponent>(id)) {
                event.finalScore = registry.get<ScoreComponent>(id).value;
            }
            event.level = 1;
            eventBus_.emit(std::move(event));
            gameOverTriggered_ = true;
            return;
        }
    }
}
