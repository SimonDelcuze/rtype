#pragma once

template <typename... Components>
ViewIterator<Components...>::ViewIterator(Registry* registry, EntityId id,
                                          const std::vector<std::size_t>* componentIndices)
    : registry_(registry), currentId_(id), componentIndices_(componentIndices)
{
    if (currentId_ < registry_->entityCount() && !matches()) {
        advance();
    }
}

template <typename... Components> ViewIterator<Components...>& ViewIterator<Components...>::operator++()
{
    ++currentId_;
    advance();
    return *this;
}

template <typename... Components> ViewIterator<Components...> ViewIterator<Components...>::operator++(int)
{
    ViewIterator tmp = *this;
    ++(*this);
    return tmp;
}

template <typename... Components> EntityId ViewIterator<Components...>::operator*() const
{
    return currentId_;
}

template <typename... Components> bool ViewIterator<Components...>::operator==(const ViewIterator& other) const
{
    return currentId_ == other.currentId_ && registry_ == other.registry_;
}

template <typename... Components> bool ViewIterator<Components...>::operator!=(const ViewIterator& other) const
{
    return !(*this == other);
}

template <typename... Components> void ViewIterator<Components...>::advance()
{
    while (currentId_ < registry_->entityCount() && !matches()) {
        ++currentId_;
    }
}

template <typename... Components> bool ViewIterator<Components...>::matches() const
{
    if (!registry_->isAlive(currentId_)) {
        return false;
    }
    for (std::size_t index : *componentIndices_) {
        if (!registry_->hasSignatureBit(currentId_, index)) {
            return false;
        }
    }
    return true;
}
