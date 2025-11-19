#pragma once

#include <stdexcept>

template <typename Component, typename... Args> Component& Registry::emplace(EntityId id, Args&&... args)
{
    if (!isAlive(id))
        throw RegistryError("Cannot emplace component on dead entity");
    return ensureStorage<Component>()->emplace(id, std::forward<Args>(args)...);
}

template <typename Component> bool Registry::has(EntityId id) const
{
    if (!isAlive(id))
        return false;
    if (const auto* storage = findStorage<Component>())
        return storage->contains(id);
    return false;
}

template <typename Component> Component& Registry::get(EntityId id)
{
    if (!isAlive(id))
        throw RegistryError("Cannot get component from dead entity");
    auto* storage = findStorage<Component>();
    if (storage == nullptr)
        throw ComponentNotFoundError("Component type not registered");
    return storage->fetch(id);
}

template <typename Component> const Component& Registry::get(EntityId id) const
{
    if (!isAlive(id))
        throw RegistryError("Cannot get component from dead entity");
    const auto* storage = findStorage<Component>();
    if (storage == nullptr)
        throw ComponentNotFoundError("Component type not registered");
    return storage->fetch(id);
}

template <typename Component> void Registry::remove(EntityId id)
{
    if (!isAlive(id))
        return;
    if (auto* storage = findStorage<Component>())
        storage->remove(id);
}

template <typename Component> ComponentStorage<Component>* Registry::findStorage()
{
    const auto it = storages_.find(std::type_index(typeid(Component)));
    if (it == storages_.end())
        return nullptr;
    return static_cast<ComponentStorage<Component>*>(it->second.get());
}

template <typename Component> const ComponentStorage<Component>* Registry::findStorage() const
{
    const auto it = storages_.find(std::type_index(typeid(Component)));
    if (it == storages_.end())
        return nullptr;
    return static_cast<const ComponentStorage<Component>*>(it->second.get());
}

template <typename Component> ComponentStorage<Component>* Registry::ensureStorage()
{
    if (auto* storage = findStorage<Component>())
        return storage;
    auto storage = std::make_unique<ComponentStorage<Component>>();
    auto* raw    = storage.get();
    storages_.emplace(std::type_index(typeid(Component)), std::move(storage));
    return raw;
}

template <typename Component>
template <typename... Args>
Component& ComponentStorage<Component>::emplace(EntityId id, Args&&... args)
{
    auto result = data.insert_or_assign(id, Component(std::forward<Args>(args)...));
    return result.first->second;
}

template <typename Component> bool ComponentStorage<Component>::contains(EntityId id) const
{
    return data.find(id) != data.end();
}

template <typename Component> Component& ComponentStorage<Component>::fetch(EntityId id)
{
    auto it = data.find(id);
    if (it == data.end())
        throw ComponentNotFoundError("Requested component not found on entity");
    return it->second;
}

template <typename Component> const Component& ComponentStorage<Component>::fetch(EntityId id) const
{
    auto it = data.find(id);
    if (it == data.end())
        throw ComponentNotFoundError("Requested component not found on entity");
    return it->second;
}

template <typename Component> void ComponentStorage<Component>::remove(EntityId id)
{
    data.erase(id);
}
