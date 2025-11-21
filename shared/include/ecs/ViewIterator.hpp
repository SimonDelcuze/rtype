#pragma once

#include "ecs/Registry.hpp"

#include <cstddef>
#include <vector>

template <typename... Components> class ViewIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = EntityId;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const EntityId*;
    using reference         = EntityId;

    ViewIterator(Registry* registry, EntityId id, const std::vector<std::size_t>* componentIndices);

    ViewIterator& operator++();
    ViewIterator operator++(int);

    EntityId operator*() const;

    bool operator==(const ViewIterator& other) const;
    bool operator!=(const ViewIterator& other) const;

  private:
    void advance();
    bool matches() const;

    Registry* registry_;
    EntityId currentId_;
    const std::vector<std::size_t>* componentIndices_;
};

#include "ecs/ViewIterator.tpp"
