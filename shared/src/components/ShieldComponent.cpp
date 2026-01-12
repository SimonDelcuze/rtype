#include "components/ShieldComponent.hpp"

ShieldComponent ShieldComponent::create(std::uint32_t owner)
{
    ShieldComponent comp;
    comp.ownerId       = owner;
    comp.hitsRemaining = kMaxHits;
    return comp;
}
