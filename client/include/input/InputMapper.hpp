#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <cstdint>

class InputMapper
{
  public:
    static constexpr std::uint16_t UpFlag    = 1u << 0;
    static constexpr std::uint16_t DownFlag  = 1u << 1;
    static constexpr std::uint16_t LeftFlag  = 1u << 2;
    static constexpr std::uint16_t RightFlag = 1u << 3;
    static constexpr std::uint16_t FireFlag  = 1u << 4;

    virtual ~InputMapper() = default;
    void handleEvent(const sf::Event& event);
    virtual std::uint16_t pollFlags() const;

  private:
    void setKeyState(sf::Keyboard::Key key, bool pressed);

    bool upPressed_    = false;
    bool downPressed_  = false;
    bool leftPressed_  = false;
    bool rightPressed_ = false;
    bool firePressed_  = false;
};
