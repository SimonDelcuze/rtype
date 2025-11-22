#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

enum class UdpError
{
    None,
    WouldBlock,
    Interrupted,
    MessageTooLong,
    NetworkDown,
    AddrNotAvail,
    Perm,
    NoMem,
    NotOpen,
    InvalidArgument,
    Unknown
};

struct UdpResult
{
    std::size_t size;
    UdpError error;
    bool ok() const
    {
        return error == UdpError::None;
    }
};

struct IpEndpoint
{
    std::array<std::uint8_t, 4> addr;
    std::uint16_t port;
    static IpEndpoint v4(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d, std::uint16_t p)
    {
        return IpEndpoint{{a, b, c, d}, p};
    }
};

class UdpSocket
{
  public:
    UdpSocket();
    ~UdpSocket();
    bool open(const IpEndpoint& bindTo);
    void close();
    bool isOpen() const;
    bool setNonBlocking(bool enable);
    bool setRecvBuffer(int bytes);
    bool setSendBuffer(int bytes);
    UdpResult sendTo(const std::uint8_t* data, std::size_t len, const IpEndpoint& dst);
    UdpResult recvFrom(std::uint8_t* buf, std::size_t len, IpEndpoint& src);
    IpEndpoint localEndpoint() const;

  private:
    int fd_;
};
