#include "runtime/MenuMusic.hpp"

#include "Logger.hpp"

#include <SFML/Audio/Music.hpp>

namespace
{
    sf::Music g_music;
    bool g_loaded = false;

    void ensureLoaded()
    {
        if (g_loaded)
            return;
        g_loaded = g_music.openFromFile("client/assets/music/theme.ogg");
        if (!g_loaded) {
            Logger::instance().warn(
                "Failed to load launcher music at client/assets/music/theme.ogg (manifest id: menu_music)");
        } else {
#if defined(SFML_VERSION_MAJOR) && SFML_VERSION_MAJOR >= 3
            g_music.setLooping(true);
#else
            g_music.setLoop(true);
#endif
        }
    }
} // namespace

void startLauncherMusic(float volume)
{
    ensureLoaded();
    if (!g_loaded)
        return;
    g_music.setVolume(volume);
    if (g_music.getStatus() != sf::SoundSource::Status::Playing)
        g_music.play();
}

void setLauncherMusicVolume(float volume)
{
    if (!g_loaded)
        return;
    g_music.setVolume(volume);
}

void stopLauncherMusic()
{
    if (!g_loaded)
        return;
    if (g_music.getStatus() == sf::SoundSource::Status::Playing)
        g_music.stop();
}

bool isLauncherMusicPlaying()
{
    if (!g_loaded)
        return false;
    return g_music.getStatus() == sf::SoundSource::Status::Playing;
}
