#include "systems/GameOverSystem.hpp"

#include "Logger.hpp"
#include "components/CameraComponent.hpp"
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

EntityId GameOverSystem::findAlivePlayer(Registry& registry)
{
    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);

        if (!tag.hasTag(EntityTag::Player)) {
            continue;
        }

        const auto& lives = registry.get<LivesComponent>(id);
        if (!lives.isDead()) {
            return id;
        }
    }

    return std::numeric_limits<EntityId>::max();
}

void GameOverSystem::enableSpectateMode(Registry& registry, EntityId targetPlayerId)
{
    for (EntityId cameraId : registry.view<CameraComponent>()) {
        if (!registry.isAlive(cameraId)) {
            continue;
        }

        auto& camera = registry.get<CameraComponent>(cameraId);
        if (camera.active) {
            camera.setTarget(targetPlayerId, 5.0F);
            Logger::instance().info("[GameOverSystem] Spectating player " + std::to_string(targetPlayerId));
            spectatingPlayerId_ = targetPlayerId;
            return;
        }
    }
}

void GameOverSystem::update(Registry& registry, float deltaTime)
{
    (void) deltaTime;

    if (gameOverTriggered_) {
        return;
    }

    EntityId localPlayerEntity = std::numeric_limits<EntityId>::max();
    bool localPlayerFound      = false;

    for (EntityId id : registry.view<TagComponent, LivesComponent>()) {
        const auto& tag = registry.get<TagComponent>(id);

        if (!tag.hasTag(EntityTag::Player)) {
            continue;
        }

        if (registry.has<OwnershipComponent>(id)) {
            const auto& owner = registry.get<OwnershipComponent>(id);
            if (owner.ownerId == localPlayerId_) {
                localPlayerFound  = true;
                localPlayerEntity = id;
            }
        }
    }

    if (!localPlayerFound) {
        return;
    }

    const auto& localLives = registry.get<LivesComponent>(localPlayerEntity);

    if (localLives.isDead() && !localPlayerDead_) {
        localPlayerDead_ = true;
        Logger::instance().info("[GameOverSystem] Local player died, checking for alive players to spectate");

        EntityId alivePlayer = findAlivePlayer(registry);
        if (alivePlayer != std::numeric_limits<EntityId>::max()) {
            enableSpectateMode(registry, alivePlayer);
        }
    }

    if (localPlayerDead_ && spectatingPlayerId_ != std::numeric_limits<EntityId>::max()) {
        if (registry.isAlive(spectatingPlayerId_) && registry.has<LivesComponent>(spectatingPlayerId_)) {
            const auto& spectatedLives = registry.get<LivesComponent>(spectatingPlayerId_);
            if (spectatedLives.isDead()) {
                Logger::instance().info("[GameOverSystem] Spectated player died, finding another alive player");
                EntityId nextAlivePlayer = findAlivePlayer(registry);
                if (nextAlivePlayer != std::numeric_limits<EntityId>::max()) {
                    enableSpectateMode(registry, nextAlivePlayer);
                }
            }
        }
    }

    EntityId anyAlivePlayer = findAlivePlayer(registry);
    if (anyAlivePlayer == std::numeric_limits<EntityId>::max()) {
        Logger::instance().info("[GameOverSystem] All players are dead, triggering game over");

        GameOverEvent event{};
        event.victory = false;
        event.level   = 1;

        for (EntityId id : registry.view<TagComponent, ScoreComponent>()) {
            const auto& tag = registry.get<TagComponent>(id);
            if (tag.hasTag(EntityTag::Player)) {
                int score = registry.get<ScoreComponent>(id).value;
                event.playerScores.push_back({static_cast<std::uint32_t>(id), score});
            }
        }

        eventBus_.emit(std::move(event));
        gameOverTriggered_ = true;
    }
}
