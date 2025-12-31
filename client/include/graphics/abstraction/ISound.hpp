#pragma once

#include "ISoundBuffer.hpp"

class ISound {
public:
    enum Status { Stopped, Paused, Playing };

    virtual ~ISound() = default;
    
    virtual void setBuffer(const ISoundBuffer& buffer) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual Status getStatus() const = 0;
    
    virtual void setVolume(float volume) = 0;
    virtual float getVolume() const = 0;
    virtual void setLoop(bool loop) = 0;
    virtual bool getLoop() const = 0;
};
