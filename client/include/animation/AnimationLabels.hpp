#pragma once

#include <string>
#include <unordered_map>

struct AnimationLabels
{
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> labels;

    const std::string* labelFor(const std::string& spriteId, const std::string& name) const
    {
        auto it = labels.find(spriteId);
        if (it == labels.end())
            return nullptr;
        auto lit = it->second.find(name);
        if (lit == it->second.end())
            return nullptr;
        return &lit->second;
    }
};
