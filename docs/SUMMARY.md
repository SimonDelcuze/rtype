# Table of contents

## Getting started

* [Introduction](README.md)

## Global overview

* [Introduction](global-overview/introduction.md)
* [Installation Guide](global-overview/installation-guide.md)
* [ECS Architecture](global-overview/ecs-architecture/README.md)
  * [Registry](global-overview/ecs-architecture/registry.md)
  * [Components Overview](global-overview/ecs-architecture/components-overview.md)
  * [Systems Overview](global-overview/ecs-architecture/systems-overview.md)
  * [Scheduler Architecture](global-overview/ecs-architecture/scheduler-architecture.md)
  * [Event Bus](global-overview/ecs-architecture/event-bus.md)

## Server

* [Introduction](server/introduction.md)
* [Architecture Overview](server/architecture-overview.md)
* [Threads](server/threads/README.md)
  * [Receive Thread](server/threads/receive-thread.md)
  * [Game Loop Thread (Scheduler)](server/threads/game-loop-thread-scheduler.md)
  * [Send Thread](server/threads/send-thread.md)
* [Components](server/components.md)
  * [TransformComponent](server/components/transform-component.md)
  * [VelocityComponent](server/components/velocity-component.md)
  * [HitboxComponent](server/components/hitbox-component.md)
  * [HealthComponent](server/components/health-component.md)
  * [PlayerInputComponent](server/components/player-input-component.md)
  * [MonsterAIComponent](server/components/monster-ai-component.md)
  * [MissileComponent](server/components/missile-component.md)
  * [MovementComponent](server/components/movement-component.md)
* [Systems](server/systems.md)
* [Networking](server/networking/README.md)
  * [Packet Header](server/networking/packet-header.md)
  * [Entity/Event Packets](server/networking/entity-events.md)

## Client

* [Introduction](client/introduction.md)
* [Architecture Overview](client/architecture-overview.md)
* [Threads](client/threads/README.md)
  * [Receive Thread](client/threads/receive-thread.md)
  * [Game Loop Thread (Scheduler)](client/threads/game-loop-thread-scheduler.md)
  * [Send Thread](client/threads/send-thread.md)
* [Prediction & Reconciliation](client/prediction-and-reconciliation.md)
* [EventBus: Publish/Subscribe](client/event-bus.md)
* [Asset Management](asset-manifest.md)
* [Scene Graph & Layering](client/scene-graph-layering.md)
* [Collision Masks & Hitboxes](collision-masks.md)
* [Error Handling](client/error-handling.md)
* [Components](client/components.md)
  * [AnimationComponent](client/components/animation-component.md)
  * [SpriteComponent](client/components/sprite-component.md)
  * [AudioComponent](client/components/audio-component.md)
  * [ScoreComponent](client/components/score-component.md)
  * [LivesComponent](client/components/lives-component.md)
  * [TextComponent](client/components/text-component.md)
* [Systems](client/systems.md)
  * [AnimationSystem](client/systems/animation-system.md)
  * [AudioSystem](client/systems/audio-system.md)
  * [CameraSystem](client/camera-system.md)
  * [InterpolationSystem](client/systems/interpolation-system.md)
  * [ReconciliationSystem](client/reconciliation-system.md)
  * [HUDSystem](client/systems/hud-system.md)
