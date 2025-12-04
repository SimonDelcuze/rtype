#include "level/EntityTypeRegistry.hpp"

void EntityTypeRegistry::registerType(std::uint16_t typeId, RenderTypeData data)
{
    types_[typeId] = data;
}

const RenderTypeData* EntityTypeRegistry::get(std::uint16_t typeId) const
{
    auto it = types_.find(typeId);
    if (it == types_.end()) {
        return nullptr;
    }
    return &it->second;
}

bool EntityTypeRegistry::has(std::uint16_t typeId) const
{
    return types_.find(typeId) != types_.end();
}

void EntityTypeRegistry::clear()
{
    types_.clear();
}

std::size_t EntityTypeRegistry::size() const
{
    return types_.size();
}
