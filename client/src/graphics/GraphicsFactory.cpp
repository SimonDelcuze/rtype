#include "graphics/GraphicsFactory.hpp"

#include "graphics/backends/sfml/SFMLFont.hpp"
#include "graphics/backends/sfml/SFMLMusic.hpp"
#include "graphics/backends/sfml/SFMLSound.hpp"
#include "graphics/backends/sfml/SFMLSoundBuffer.hpp"
#include "graphics/backends/sfml/SFMLSprite.hpp"
#include "graphics/backends/sfml/SFMLText.hpp"
#include "graphics/backends/sfml/SFMLTexture.hpp"
#include "graphics/backends/sfml/SFMLWindow.hpp"

std::unique_ptr<IWindow> GraphicsFactory::createWindow(unsigned int width, unsigned int height,
                                                       const std::string& title)
{
    return std::make_unique<SFMLWindow>(width, height, title);
}

std::unique_ptr<ITexture> GraphicsFactory::createTexture()
{
    return std::make_unique<SFMLTexture>();
}

std::unique_ptr<ISprite> GraphicsFactory::createSprite()
{
    return std::make_unique<SFMLSprite>();
}

std::unique_ptr<IText> GraphicsFactory::createText()
{
    return std::make_unique<SFMLText>();
}

std::unique_ptr<IFont> GraphicsFactory::createFont()
{
    return std::make_unique<SFMLFont>();
}

std::unique_ptr<ISoundBuffer> GraphicsFactory::createSoundBuffer()
{
    return std::make_unique<SFMLSoundBuffer>();
}

std::unique_ptr<ISound> GraphicsFactory::createSound()
{
    return std::make_unique<SFMLSound>();
}

std::unique_ptr<IMusic> GraphicsFactory::createMusic()
{
    return std::make_unique<SFMLMusic>();
}
