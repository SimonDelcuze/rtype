#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <cstdint>
#include <network/InputPacket.hpp>

class InputMapper
{
  public:
    static constexpr std::uint16_t UpFlag    = 1u << 0;
    static constexpr std::uint16_t DownFlag  = 1u << 1;
    static constexpr std::uint16_t LeftFlag  = 1u << 2;
    static constexpr std::uint16_t RightFlag = 1u << 3;
    static constexpr std::uint16_t FireFlag  = 1u << 4;
    static constexpr std::uint16_t Charge1Flag =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(InputFlag::Charge1));
    static constexpr std::uint16_t Charge2Flag =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(InputFlag::Charge2));
    static constexpr std::uint16_t Charge3Flag =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(InputFlag::Charge3));
    static constexpr std::uint16_t Charge4Flag =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(InputFlag::Charge4));
    static constexpr std::uint16_t Charge5Flag =
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(InputFlag::Charge5));

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
