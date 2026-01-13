#include "systems/GameOverSystem.hpp"

#include "Logger.hpp"
#include "components/LivesComponent.hpp"
#include "components/OwnershipComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "components/TagComponent.hpp"
#include "ecs/Registry.hpp"
#include "events/GameEvents.hpp"

GameOverSystem::GameOverSystem(EventBus& eventBus, std::uint32_t localPlayerId, RoomType gameMode)
    : eventBus_(eventBus), localPlayerId_(localPlayerId), gameMode_(gameMode)
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

        bool isLocalPlayer = false;
        if (registry.has<OwnershipComponent>(id)) {
            const auto& owner = registry.get<OwnershipComponent>(id);
            if (owner.ownerId == localPlayerId_) {
                isLocalPlayer = true;
            }
        }

        if (!isLocalPlayer) {
            continue;
        }

        playerIds.push_back(id);
        const auto& lives = registry.get<LivesComponent>(id);

        if (lives.isDead()) {
            GameOverEvent event{};
            event.victory = false;
            int score     = 0;
            if (registry.has<ScoreComponent>(id)) {
                score = registry.get<ScoreComponent>(id).value;
            }
            event.playerScores.push_back({static_cast<std::uint32_t>(id), score});
            event.level = 1;
            eventBus_.emit(std::move(event));
            gameOverTriggered_ = true;
            return;
        }
    }
}
