#pragma once

#include "graphics/abstraction/IFont.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class FontManager
{
  public:
    const IFont& load(const std::string& id, const std::string& path);
    std::shared_ptr<IFont> get(const std::string& id) const;
    bool has(const std::string& id) const;
    void remove(const std::string& id);
    void clear();
    std::size_t size() const;

  private:
    std::unordered_map<std::string, std::shared_ptr<IFont>> fonts_;
};
