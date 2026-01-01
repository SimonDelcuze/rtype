#pragma once

#include "abstraction/IFont.hpp"
#include "abstraction/IMusic.hpp"
#include "abstraction/ISound.hpp"
#include "abstraction/ISoundBuffer.hpp"
#include "abstraction/ISprite.hpp"
#include "abstraction/IText.hpp"
#include "abstraction/ITexture.hpp"
#include "abstraction/IWindow.hpp"

#include <memory>
#include <string>

class GraphicsFactory
{
  public:
    std::unique_ptr<IWindow> createWindow(unsigned int width, unsigned int height, const std::string& title);
    std::unique_ptr<ITexture> createTexture();
    std::unique_ptr<ISprite> createSprite();
    std::unique_ptr<IText> createText();
    std::unique_ptr<IFont> createFont();
    std::unique_ptr<ISoundBuffer> createSoundBuffer();
    std::unique_ptr<ISound> createSound();
    std::unique_ptr<IMusic> createMusic();
};
