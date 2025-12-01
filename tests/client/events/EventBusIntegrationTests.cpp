#include "events/AudioEvents.hpp"
#include "events/EventBus.hpp"
#include "events/GameEvents.hpp"
#include "events/RenderEvents.hpp"
#include "events/UIEvents.hpp"

#include <gtest/gtest.h>

class EventBusIntegrationTests : public ::testing::Test
{
  protected:
    EventBus bus;
};

TEST_F(EventBusIntegrationTests, EntityDamagedEventFlow)
{
    bool called     = false;
    EntityId target = 0;
    int damage      = 0;

    bus.subscribe<EntityDamagedEvent>([&](const EntityDamagedEvent& e) {
        called = true;
        target = e.entityId;
        damage = e.damageAmount;
    });

    bus.emit<EntityDamagedEvent>(EntityDamagedEvent{42u, 1u, 25, 75});

    EXPECT_FALSE(called);

    bus.process();

    EXPECT_TRUE(called);
    EXPECT_EQ(target, 42u);
    EXPECT_EQ(damage, 25);
}

TEST_F(EventBusIntegrationTests, PlaySoundEventFlow)
{
    bool called = false;
    std::string soundName;
    float volume = 0.0F;

    bus.subscribe<PlaySoundEvent>([&](const PlaySoundEvent& e) {
        called    = true;
        soundName = e.soundName;
        volume    = e.volume;
    });

    bus.emit<PlaySoundEvent>(PlaySoundEvent{"explosion.wav", 0.9F, 1.0F, false});

    bus.process();

    EXPECT_TRUE(called);
    EXPECT_EQ(soundName, "explosion.wav");
    EXPECT_FLOAT_EQ(volume, 0.9F);
}

TEST_F(EventBusIntegrationTests, MultipleSubscribersReceiveSameEvent)
{
    int callCount = 0;

    bus.subscribe<PlayerScoredEvent>([&](const PlayerScoredEvent&) { callCount++; });
    bus.subscribe<PlayerScoredEvent>([&](const PlayerScoredEvent&) { callCount++; });
    bus.subscribe<PlayerScoredEvent>([&](const PlayerScoredEvent&) { callCount++; });

    bus.emit<PlayerScoredEvent>(PlayerScoredEvent{1u, 100, 1000, "enemy_kill"});

    bus.process();

    EXPECT_EQ(callCount, 3);
}

TEST_F(EventBusIntegrationTests, DifferentEventTypesAreIndependent)
{
    bool audioEventCalled  = false;
    bool renderEventCalled = false;

    bus.subscribe<PlaySoundEvent>([&](const PlaySoundEvent&) { audioEventCalled = true; });
    bus.subscribe<CameraShakeEvent>([&](const CameraShakeEvent&) { renderEventCalled = true; });

    bus.emit<PlaySoundEvent>(PlaySoundEvent{"test.wav"});

    bus.process();

    EXPECT_TRUE(audioEventCalled);
    EXPECT_FALSE(renderEventCalled);
}

TEST_F(EventBusIntegrationTests, EventsProcessedInOrder)
{
    std::vector<int> order;

    bus.subscribe<EntitySpawnedEvent>([&](const EntitySpawnedEvent&) { order.push_back(1); });

    bus.emit<EntitySpawnedEvent>(EntitySpawnedEvent{1u, 0.0F, 0.0F, "player"});
    bus.emit<EntitySpawnedEvent>(EntitySpawnedEvent{2u, 10.0F, 10.0F, "enemy"});
    bus.emit<EntitySpawnedEvent>(EntitySpawnedEvent{3u, 20.0F, 20.0F, "powerup"});

    bus.process();

    EXPECT_EQ(order.size(), 3u);
}

TEST_F(EventBusIntegrationTests, EmitDuringProcessIsDeferred)
{
    std::vector<std::string> order;

    bus.subscribe<EntityDamagedEvent>([&](const EntityDamagedEvent&) {
        order.push_back("damage");
        bus.emit<PlaySoundEvent>(PlaySoundEvent{"hit.wav"});
    });

    bus.subscribe<PlaySoundEvent>([&](const PlaySoundEvent&) { order.push_back("sound"); });

    bus.emit<EntityDamagedEvent>(EntityDamagedEvent{1u, 2u, 10, 50});

    bus.process();

    EXPECT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], "damage");

    bus.process();

    EXPECT_EQ(order.size(), 2u);
    EXPECT_EQ(order[1], "sound");
}

TEST_F(EventBusIntegrationTests, MultipleEventsInSingleFrame)
{
    int damageCount = 0;
    int scoreCount  = 0;
    int spawnCount  = 0;

    bus.subscribe<EntityDamagedEvent>([&](const EntityDamagedEvent&) { damageCount++; });
    bus.subscribe<PlayerScoredEvent>([&](const PlayerScoredEvent&) { scoreCount++; });
    bus.subscribe<EntitySpawnedEvent>([&](const EntitySpawnedEvent&) { spawnCount++; });

    bus.emit<EntityDamagedEvent>(EntityDamagedEvent{1u, 2u, 10, 50});
    bus.emit<PlayerScoredEvent>(PlayerScoredEvent{1u, 100, 1000, "kill"});
    bus.emit<EntitySpawnedEvent>(EntitySpawnedEvent{3u, 0.0F, 0.0F, "enemy"});
    bus.emit<EntityDamagedEvent>(EntityDamagedEvent{4u, 1u, 20, 30});

    bus.process();

    EXPECT_EQ(damageCount, 2);
    EXPECT_EQ(scoreCount, 1);
    EXPECT_EQ(spawnCount, 1);
}

TEST_F(EventBusIntegrationTests, ClearRemovesPendingEvents)
{
    bool called = false;

    bus.subscribe<PlaySoundEvent>([&](const PlaySoundEvent&) { called = true; });

    bus.emit<PlaySoundEvent>(PlaySoundEvent{"test.wav"});

    bus.clear();

    bus.process();

    EXPECT_FALSE(called);
}

TEST_F(EventBusIntegrationTests, UINotificationEvent)
{
    std::string message;
    ShowNotificationEvent::Type type = ShowNotificationEvent::Type::Info;

    bus.subscribe<ShowNotificationEvent>([&](const ShowNotificationEvent& e) {
        message = e.message;
        type    = e.type;
    });

    bus.emit<ShowNotificationEvent>(
        ShowNotificationEvent{"Level Complete!", 5.0F, ShowNotificationEvent::Type::Success});

    bus.process();

    EXPECT_EQ(message, "Level Complete!");
    EXPECT_EQ(type, ShowNotificationEvent::Type::Success);
}

TEST_F(EventBusIntegrationTests, CameraShakeEvent)
{
    float intensity = 0.0F;
    float duration  = 0.0F;

    bus.subscribe<CameraShakeEvent>([&](const CameraShakeEvent& e) {
        intensity = e.intensity;
        duration  = e.duration;
    });

    bus.emit<CameraShakeEvent>(CameraShakeEvent{8.0F, 0.5F, 30.0F});

    bus.process();

    EXPECT_FLOAT_EQ(intensity, 8.0F);
    EXPECT_FLOAT_EQ(duration, 0.5F);
}

TEST_F(EventBusIntegrationTests, BossEventsFlow)
{
    int eventsReceived = 0;
    std::string bossName;

    bus.subscribe<BossSpawnedEvent>([&](const BossSpawnedEvent& e) {
        eventsReceived++;
        bossName = e.bossName;
    });

    bus.subscribe<BossDefeatedEvent>([&](const BossDefeatedEvent&) { eventsReceived++; });

    bus.emit<BossSpawnedEvent>(BossSpawnedEvent{100u, "Mega Boss", 10000});
    bus.process();

    EXPECT_EQ(eventsReceived, 1);
    EXPECT_EQ(bossName, "Mega Boss");

    bus.emit<BossDefeatedEvent>(BossDefeatedEvent{100u, "Mega Boss", 5000});
    bus.process();

    EXPECT_EQ(eventsReceived, 2);
}

TEST_F(EventBusIntegrationTests, WaveSystemEvents)
{
    int waveNumber   = 0;
    int enemyCount   = 0;
    bool waveStarted = false;
    bool waveEnded   = false;

    bus.subscribe<WaveStartedEvent>([&](const WaveStartedEvent& e) {
        waveStarted = true;
        waveNumber  = e.waveNumber;
        enemyCount  = e.enemyCount;
    });

    bus.subscribe<WaveCompletedEvent>([&](const WaveCompletedEvent&) { waveEnded = true; });

    bus.emit<WaveStartedEvent>(WaveStartedEvent{5, 25});
    bus.process();

    EXPECT_TRUE(waveStarted);
    EXPECT_EQ(waveNumber, 5);
    EXPECT_EQ(enemyCount, 25);

    bus.emit<WaveCompletedEvent>(WaveCompletedEvent{5, 25, 1000});
    bus.process();

    EXPECT_TRUE(waveEnded);
}

TEST_F(EventBusIntegrationTests, PlayerLifecycleEvents)
{
    std::vector<std::string> lifecycle;

    bus.subscribe<EntitySpawnedEvent>([&](const EntitySpawnedEvent& e) {
        if (e.entityType == "player") {
            lifecycle.push_back("spawn");
        }
    });

    bus.subscribe<PlayerDiedEvent>([&](const PlayerDiedEvent&) { lifecycle.push_back("died"); });

    bus.subscribe<PlayerRespawnedEvent>([&](const PlayerRespawnedEvent&) { lifecycle.push_back("respawn"); });

    bus.emit<EntitySpawnedEvent>(EntitySpawnedEvent{1u, 0.0F, 0.0F, "player"});
    bus.process();

    bus.emit<PlayerDiedEvent>(PlayerDiedEvent{1u, 5u, 2});
    bus.process();

    bus.emit<PlayerRespawnedEvent>(PlayerRespawnedEvent{1u, 0.0F, 0.0F});
    bus.process();

    ASSERT_EQ(lifecycle.size(), 3u);
    EXPECT_EQ(lifecycle[0], "spawn");
    EXPECT_EQ(lifecycle[1], "died");
    EXPECT_EQ(lifecycle[2], "respawn");
}

TEST_F(EventBusIntegrationTests, ComplexGameplayScenario)
{
    int totalScore         = 0;
    int soundsPlayed       = 0;
    int particlesSpawned   = 0;
    int notificationsShown = 0;

    bus.subscribe<PlayerScoredEvent>([&](const PlayerScoredEvent& e) { totalScore += e.pointsGained; });

    bus.subscribe<PlaySoundEvent>([&](const PlaySoundEvent&) { soundsPlayed++; });

    bus.subscribe<SpawnParticleEffectEvent>([&](const SpawnParticleEffectEvent&) { particlesSpawned++; });

    bus.subscribe<ShowNotificationEvent>([&](const ShowNotificationEvent&) { notificationsShown++; });

    bus.emit<EntityDamagedEvent>(EntityDamagedEvent{5u, 1u, 50, 0});
    bus.emit<PlaySoundEvent>(PlaySoundEvent{"explosion.wav"});
    bus.emit<SpawnParticleEffectEvent>(SpawnParticleEffectEvent{"explosion", 100.0F, 200.0F});
    bus.emit<PlayerScoredEvent>(PlayerScoredEvent{1u, 500, 2500, "enemy_destroyed"});
    bus.emit<ShowNotificationEvent>(ShowNotificationEvent{"Enemy Destroyed!", 2.0F});

    bus.process();

    EXPECT_EQ(totalScore, 500);
    EXPECT_EQ(soundsPlayed, 1);
    EXPECT_EQ(particlesSpawned, 1);
    EXPECT_EQ(notificationsShown, 1);
}
