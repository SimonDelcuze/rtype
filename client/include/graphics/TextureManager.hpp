#pragma once

#include "graphics/abstraction/ITexture.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

class TextureManager
{
  public:
    const ITexture& load(const std::string& id, const std::string& path);
    std::shared_ptr<ITexture> get(const std::string& id) const;
    bool has(const std::string& id) const;
    void remove(const std::string& id);
    void clear();
    std::size_t size() const;

    std::shared_ptr<ITexture> getPlaceholder();
    std::shared_ptr<ITexture> getOrPlaceholder(const std::string& id);

  private:
    void createPlaceholder();

    std::unordered_map<std::string, std::shared_ptr<ITexture>> textures_;
    std::shared_ptr<ITexture> placeholder_;
};
