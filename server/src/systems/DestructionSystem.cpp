#include "systems/DestructionSystem.hpp"

DestructionSystem::DestructionSystem(EventBus& bus) : bus_(bus) {}

void DestructionSystem::update(Registry& registry, const std::vector<EntityId>& toDestroy)
{
    for (EntityId id : toDestroy) {
        if (!registry.isAlive(id))
            continue;
        registry.destroyEntity(id);
        DestroyEvent ev{id};
        bus_.emit<DestroyEvent>(ev);
    }
    bus_.process();
}
