#pragma once

#include "components/Components.hpp"
#include "ecs/Registry.hpp"

#include <cstdint>

std::uint16_t resolveEntityType(const Registry& registry, EntityId id);
