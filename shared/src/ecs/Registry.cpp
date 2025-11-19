#include "ecs/Registry.hpp"

#include <stdexcept>
#include <utility>

EntityId Registry::createEntity()
{
    if (!freeIds_.empty()) {
        const EntityId id = freeIds_.back();
        freeIds_.pop_back();
        alive_[id] = true;
        return id;
    }
    const EntityId id = nextId_++;
    alive_.push_back(true);
    return id;
}

void Registry::destroyEntity(EntityId id)
{
    if (id >= alive_.size() || !alive_[id])
        return;
    alive_[id] = false;
    freeIds_.push_back(id);
    for (auto& [_, storage] : storages_)
        storage->remove(id);
}

bool Registry::isAlive(EntityId id) const
{
    return id < alive_.size() && alive_[id];
}

void Registry::clear()
{
    storages_.clear();
    freeIds_.clear();
    alive_.clear();
    nextId_ = 0;
}
