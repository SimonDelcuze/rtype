#pragma once

#include "ecs/View.hpp"

#include <stdexcept>
#include <utility>

template <typename Component, typename... Args> Component& Registry::emplace(EntityId id, Args&&... args)
{
    if (!isAlive(id))
        throw RegistryError("Cannot emplace component on dead entity");
    const auto componentIndex = ComponentTypeId::value<Component>();
    ensureSignatureWordCount(componentIndex);
    auto* storage   = ensureStorage<Component>();
    auto& component = storage->emplace(id, std::forward<Args>(args)...);
    setSignatureBit(id, componentIndex);
    return component;
}

template <typename Component> bool Registry::has(EntityId id) const
{
    if (!isAlive(id))
        return false;
    const auto componentIndex = ComponentTypeId::value<Component>();
    if (!hasSignatureBit(id, componentIndex))
        return false;
    if (const auto* storage = findStorage<Component>())
        return storage->contains(id);
    return false;
}

template <typename Component> Component& Registry::get(EntityId id)
{
    if (!isAlive(id))
        throw RegistryError("Cannot get component from dead entity");
    const auto componentIndex = ComponentTypeId::value<Component>();
    if (!hasSignatureBit(id, componentIndex))
        throw ComponentNotFoundError("Requested component not found on entity");
    auto* storage = findStorage<Component>();
    if (storage == nullptr)
        throw ComponentNotFoundError("Component type not registered");
    return storage->fetch(id);
}

template <typename Component> const Component& Registry::get(EntityId id) const
{
    if (!isAlive(id))
        throw RegistryError("Cannot get component from dead entity");
    const auto componentIndex = ComponentTypeId::value<Component>();
    if (!hasSignatureBit(id, componentIndex))
        throw ComponentNotFoundError("Requested component not found on entity");
    const auto* storage = findStorage<Component>();
    if (storage == nullptr)
        throw ComponentNotFoundError("Component type not registered");
    return storage->fetch(id);
}

template <typename Component> void Registry::remove(EntityId id)
{
    if (!isAlive(id))
        return;
    const auto componentIndex = ComponentTypeId::value<Component>();
    if (!hasSignatureBit(id, componentIndex))
        return;
    if (auto* storage = findStorage<Component>()) {
        storage->remove(id);
        clearSignatureBit(id, componentIndex);
    }
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
    if (id >= sparse.size())
        sparse.resize(id + 1, npos);
    const auto index = sparse[id];
    if (index != npos && index < dense.size() && dense[index] == id) {
        data[index] = Component(std::forward<Args>(args)...);
        return data[index];
    }
    dense.push_back(id);
    sparse[id] = dense.size() - 1;
    data.emplace_back(std::forward<Args>(args)...);
    return data.back();
}

template <typename Component> bool ComponentStorage<Component>::contains(EntityId id) const
{
    if (id >= sparse.size())
        return false;
    const auto index = sparse[id];
    return index != npos && index < dense.size() && dense[index] == id;
}

template <typename Component> Component& ComponentStorage<Component>::fetch(EntityId id)
{
    if (!contains(id))
        throw ComponentNotFoundError("Requested component not found on entity");
    return data[sparse[id]];
}

template <typename Component> const Component& ComponentStorage<Component>::fetch(EntityId id) const
{
    if (!contains(id))
        throw ComponentNotFoundError("Requested component not found on entity");
    return data[sparse[id]];
}

template <typename Component> void ComponentStorage<Component>::remove(EntityId id)
{
    if (!contains(id))
        return;
    const std::size_t index = sparse[id];
    const std::size_t last  = dense.size() - 1;
    if (index != last) {
        dense[index]         = dense[last];
        data[index]          = std::move(data[last]);
        sparse[dense[index]] = index;
    }
    dense.pop_back();
    data.pop_back();
    sparse[id] = npos;
}

template <typename... Components> View<Components...> Registry::view()
{
    return View<Components...>(*this);
}
