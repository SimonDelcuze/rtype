#include "systems/GameOverSystem.hpp"

#include "Logger.hpp"
#include "components/LivesComponent.hpp"
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

    int playerCount = 0;
    std::vector<EntityId> playerIds;

    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);

        if (!tag.hasTag(EntityTag::Player)) {
            continue;
        }

        playerCount++;
        playerIds.push_back(id);
        const auto& lives = registry.get<LivesComponent>(id);

        if (lives.isDead()) {
            Logger::instance().info("[GameOverSystem] Player " + std::to_string(id) + " is dead (" +
                                    std::to_string(lives.current) + "/" + std::to_string(lives.max) +
                                    ") - Triggering Game Over!");
            GameOverEvent event{};
            event.victory    = false;
            event.finalScore = 0;
            event.level      = 1;
            eventBus_.emit(std::move(event));
            gameOverTriggered_ = true;
            return;
        }
    }

    static int logThrottle = 0;
    if (playerCount > 1 && logThrottle++ % 60 == 0) {
        std::string ids;
        for (auto id : playerIds) {
            ids += std::to_string(id) + " ";
        }
        Logger::instance().warn("[GameOverSystem] Found " + std::to_string(playerCount) + " player entities: " + ids);
    }
}
