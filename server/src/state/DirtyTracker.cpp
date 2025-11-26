#include "state/DirtyTracker.hpp"

#include <cmath>

namespace
{
    constexpr float kEps = 1e-4F;
}

bool DirtyTracker::moved(const TransformComponent& a, const TransformComponent& b)
{
    return std::fabs(a.x - b.x) > kEps || std::fabs(a.y - b.y) > kEps || std::fabs(a.rotation - b.rotation) > kEps ||
           std::fabs(a.scaleX - b.scaleX) > kEps || std::fabs(a.scaleY - b.scaleY) > kEps;
}

void DirtyTracker::onSpawn(EntityId id, const TransformComponent& t)
{
    flags_[id]    = flags_.count(id) ? flags_[id] | DirtyFlag::Spawned : DirtyFlag::Spawned;
    previous_[id] = t;
}

void DirtyTracker::onDestroy(EntityId id)
{
    flags_[id] = flags_.count(id) ? flags_[id] | DirtyFlag::Destroyed : DirtyFlag::Destroyed;
    previous_.erase(id);
}

void DirtyTracker::trackTransform(EntityId id, const TransformComponent& t)
{
    auto itPrev = previous_.find(id);
    if (itPrev == previous_.end()) {
        previous_[id] = t;
        return;
    }
    if (moved(itPrev->second, t)) {
        flags_[id]     = flags_.count(id) ? flags_[id] | DirtyFlag::Moved : DirtyFlag::Moved;
        itPrev->second = t;
    }
}

std::vector<DirtyEntry> DirtyTracker::consume()
{
    std::vector<DirtyEntry> out;
    out.reserve(flags_.size());
    for (auto& kv : flags_) {
        DirtyEntry e{};
        e.id     = kv.first;
        e.flags  = kv.second;
        auto itT = previous_.find(kv.first);
        if (itT != previous_.end())
            e.transform = itT->second;
        out.push_back(e);
    }
    flags_.clear();
    return out;
}
