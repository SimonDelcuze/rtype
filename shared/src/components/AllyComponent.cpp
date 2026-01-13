#include "components/AllyComponent.hpp"

AllyComponent AllyComponent::create(std::uint32_t owner)
{
    AllyComponent comp;
    comp.ownerId    = owner;
    comp.shootTimer = 0.0F;
    return comp;
}
