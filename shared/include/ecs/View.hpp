#pragma once

#include "ecs/ComponentTypeId.hpp"
#include "ecs/Registry.hpp"
#include "ecs/ViewIterator.hpp"

#include <cstddef>
#include <vector>

template <typename... Components> class View
{
  public:
    explicit View(Registry& registry);

    ViewIterator<Components...> begin();
    ViewIterator<Components...> end();

  private:
    Registry& registry_;
    std::vector<std::size_t> componentIndices_;
};

#include "ecs/View.tpp"
