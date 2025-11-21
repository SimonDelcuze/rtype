#pragma once

template <typename... Components> View<Components...>::View(Registry& registry) : registry_(registry)
{
    componentIndices_ = {ComponentTypeId::value<Components>()...};
}

template <typename... Components> ViewIterator<Components...> View<Components...>::begin()
{
    return ViewIterator<Components...>(&registry_, 0, &componentIndices_);
}

template <typename... Components> ViewIterator<Components...> View<Components...>::end()
{
    return ViewIterator<Components...>(&registry_, registry_.entityCount(), &componentIndices_);
}
