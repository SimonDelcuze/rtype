#pragma once

#include "graphics/abstraction/ISoundBuffer.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class SoundManager
{
  public:
    const ISoundBuffer& load(const std::string& id, const std::string& filepath);
    std::shared_ptr<ISoundBuffer> get(const std::string& id) const;
    bool has(const std::string& id) const;
    void remove(const std::string& id);
    void clear();

    static void setGlobalVolume(float volume);

  private:
    std::unordered_map<std::string, std::shared_ptr<ISoundBuffer>> buffers_;
};
