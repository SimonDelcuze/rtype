#include "ecs/ComponentTypeId.hpp"

std::atomic<std::size_t> ComponentTypeId::counter_{0};

std::size_t ComponentTypeId::next()
{
    return counter_.fetch_add(1);
}
