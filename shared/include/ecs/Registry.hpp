#pragma once

#include "errors/ComponentNotFoundError.hpp"
#include "errors/RegistryError.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

using EntityId = std::uint32_t;

struct ComponentStorageBase
{
    virtual ~ComponentStorageBase()  = default;
    virtual void remove(EntityId id) = 0;
};

template <typename Component> struct ComponentStorage : ComponentStorageBase
{
    std::unordered_map<EntityId, Component> data;
    template <typename... Args> Component& emplace(EntityId id, Args&&... args);
    bool contains(EntityId id) const;
    Component& fetch(EntityId id);
    const Component& fetch(EntityId id) const;
    void remove(EntityId id) override;
};

class Registry
{
  public:
    EntityId createEntity();
    void destroyEntity(EntityId id);
    bool isAlive(EntityId id) const;
    void clear();

    template <typename Component, typename... Args> Component& emplace(EntityId id, Args&&... args);
    template <typename Component> bool has(EntityId id) const;
    template <typename Component> Component& get(EntityId id);
    template <typename Component> const Component& get(EntityId id) const;
    template <typename Component> void remove(EntityId id);

  private:
    template <typename Component> ComponentStorage<Component>* findStorage();
    template <typename Component> const ComponentStorage<Component>* findStorage() const;
    template <typename Component> ComponentStorage<Component>* ensureStorage();

    std::vector<EntityId> freeIds_;
    std::vector<bool> alive_;
    EntityId nextId_ = 0;
    std::unordered_map<std::type_index, std::unique_ptr<ComponentStorageBase>> storages_;
};

#include "ecs/Registry.tpp"
