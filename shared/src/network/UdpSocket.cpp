#include "network/UdpSocket.hpp"

#include <cstdint>
#include <cstring>
#include <limits>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{
#ifdef _WIN32
    static bool wsa()
    {
        static bool inited = false;
        if (inited)
            return true;
        WSADATA w;
        if (WSAStartup(MAKEWORD(2, 2), &w) != 0)
            return false;
        inited = true;
        return true;
    }
    static sockaddr_in toSockaddr(const IpEndpoint& ep)
    {
        sockaddr_in sa{};
        sa.sin_family   = AF_INET;
        sa.sin_port     = htons(ep.port);
        std::uint32_t v = (static_cast<std::uint32_t>(ep.addr[0]) << 24) |
                          (static_cast<std::uint32_t>(ep.addr[1]) << 16) |
                          (static_cast<std::uint32_t>(ep.addr[2]) << 8) | (static_cast<std::uint32_t>(ep.addr[3]));
        sa.sin_addr.s_addr = htonl(v);
        return sa;
    }
    static IpEndpoint fromSockaddr(const sockaddr_in& sa)
    {
        std::uint32_t n = ntohl(sa.sin_addr.s_addr);
        IpEndpoint ep{};
        ep.addr = {static_cast<std::uint8_t>((n >> 24) & 0xFF), static_cast<std::uint8_t>((n >> 16) & 0xFF),
                   static_cast<std::uint8_t>((n >> 8) & 0xFF), static_cast<std::uint8_t>(n & 0xFF)};
        ep.port = ntohs(sa.sin_port);
        return ep;
    }
    static UdpError mapErr(int e)
    {
        if (e == WSAEWOULDBLOCK)
            return UdpError::WouldBlock;
        if (e == WSAEINTR)
            return UdpError::Interrupted;
        if (e == WSAEMSGSIZE)
            return UdpError::MessageTooLong;
        if (e == WSAENETDOWN || e == WSAENETUNREACH || e == WSAEHOSTUNREACH)
            return UdpError::NetworkDown;
        if (e == WSAEADDRNOTAVAIL)
            return UdpError::AddrNotAvail;
        if (e == WSAEACCES)
            return UdpError::Perm;
        if (e == WSAENOBUFS)
            return UdpError::NoMem;
        if (e == WSAENOTSOCK)
            return UdpError::NotOpen;
        if (e == WSAEINVAL)
            return UdpError::InvalidArgument;
        return UdpError::Unknown;
    }
#else
    static sockaddr_in toSockaddr(const IpEndpoint& ep)
    {
        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port   = htons(ep.port);
        sa.sin_addr.s_addr =
            htonl((static_cast<std::uint32_t>(ep.addr[0]) << 24) | (static_cast<std::uint32_t>(ep.addr[1]) << 16) |
                  (static_cast<std::uint32_t>(ep.addr[2]) << 8) | (static_cast<std::uint32_t>(ep.addr[3])));
        return sa;
    }
    static IpEndpoint fromSockaddr(const sockaddr_in& sa)
    {
        std::uint32_t n = ntohl(sa.sin_addr.s_addr);
        IpEndpoint ep{};
        ep.addr = {static_cast<std::uint8_t>((n >> 24) & 0xFF), static_cast<std::uint8_t>((n >> 16) & 0xFF),
                   static_cast<std::uint8_t>((n >> 8) & 0xFF), static_cast<std::uint8_t>(n & 0xFF)};
        ep.port = ntohs(sa.sin_port);
        return ep;
    }
    static UdpError mapErr(int e)
    {
        if (e == EWOULDBLOCK || e == EAGAIN)
            return UdpError::WouldBlock;
        if (e == EINTR)
            return UdpError::Interrupted;
        if (e == EMSGSIZE)
            return UdpError::MessageTooLong;
        if (e == ENETDOWN || e == ENETUNREACH || e == EHOSTUNREACH)
            return UdpError::NetworkDown;
        if (e == EADDRNOTAVAIL)
            return UdpError::AddrNotAvail;
        if (e == EACCES || e == EPERM)
            return UdpError::Perm;
        if (e == ENOBUFS)
            return UdpError::NoMem;
        if (e == EBADF)
            return UdpError::NotOpen;
        if (e == EINVAL)
            return UdpError::InvalidArgument;
        return UdpError::Unknown;
    }
#endif
} // namespace

UdpSocket::UdpSocket() : fd_(-1) {}

UdpSocket::~UdpSocket()
{
    close();
}

bool UdpSocket::open(const IpEndpoint& bindTo)
{
#ifdef _WIN32
    if (!wsa())
        return false;
    if (fd_ != -1)
        close();
    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
        return false;
    int yes = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
    sockaddr_in sa = toSockaddr(bindTo);
    if (::bind(s, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR) {
        ::closesocket(s);
        return false;
    }
    fd_ = static_cast<std::intptr_t>(s);
    setNonBlocking(true);
    return true;
#else
    if (fd_ != -1)
        close();
    int s = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        return false;
    int yes = 1;
    ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in sa = toSockaddr(bindTo);
    if (::bind(s, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
        ::close(s);
        return false;
    }
    fd_ = s;
    setNonBlocking(true);
    return true;
#endif
}

void UdpSocket::close()
{
#ifdef _WIN32
    if (fd_ != -1) {
        SOCKET s = static_cast<SOCKET>(fd_);
        ::closesocket(s);
        fd_ = -1;
    }
#else
    if (fd_ != -1) {
        ::close(static_cast<int>(fd_));
        fd_ = -1;
    }
#endif
}

bool UdpSocket::isOpen() const
{
    return fd_ != -1;
}

bool UdpSocket::setNonBlocking(bool enable)
{
    if (fd_ == -1)
        return false;
#ifdef _WIN32
    SOCKET s    = static_cast<SOCKET>(fd_);
    u_long mode = enable ? 1UL : 0UL;
    return ::ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    int flags = ::fcntl(static_cast<int>(fd_), F_GETFL, 0);
    if (flags == -1)
        return false;
    if (enable)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return ::fcntl(static_cast<int>(fd_), F_SETFL, flags) == 0;
#endif
}

bool UdpSocket::setRecvBuffer(int bytes)
{
    if (fd_ == -1)
        return false;
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(fd_);
    return ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bytes), sizeof(bytes)) == 0;
#else
    return ::setsockopt(static_cast<int>(fd_), SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes)) == 0;
#endif
}

bool UdpSocket::setSendBuffer(int bytes)
{
    if (fd_ == -1)
        return false;
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(fd_);
    return ::setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&bytes), sizeof(bytes)) == 0;
#else
    return ::setsockopt(static_cast<int>(fd_), SOL_SOCKET, SO_SNDBUF, &bytes, sizeof(bytes)) == 0;
#endif
}

UdpResult UdpSocket::sendTo(const std::uint8_t* data, std::size_t len, const IpEndpoint& dst)
{
    if (fd_ == -1)
        return {0, UdpError::NotOpen};
    sockaddr_in sa = toSockaddr(dst);
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(fd_);
    if (len > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {0, UdpError::InvalidArgument};
    int sent = ::sendto(s, reinterpret_cast<const char*>(data), static_cast<int>(len), 0,
                        reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
    if (sent == SOCKET_ERROR)
        return {0, mapErr(WSAGetLastError())};
    return {static_cast<std::size_t>(sent), UdpError::None};
#else
    ssize_t sent = ::sendto(static_cast<int>(fd_), data, len, 0, reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
    if (sent < 0)
        return {0, mapErr(errno)};
    return {static_cast<std::size_t>(sent), UdpError::None};
#endif
}

UdpResult UdpSocket::recvFrom(std::uint8_t* buf, std::size_t len, IpEndpoint& src)
{
    if (fd_ == -1)
        return {0, UdpError::NotOpen};
    sockaddr_in sa{};
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(fd_);
    int sl   = sizeof(sa);
    if (len > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {0, UdpError::InvalidArgument};
    int recvd =
        ::recvfrom(s, reinterpret_cast<char*>(buf), static_cast<int>(len), 0, reinterpret_cast<sockaddr*>(&sa), &sl);
    if (recvd == SOCKET_ERROR)
        return {0, mapErr(WSAGetLastError())};
    src = fromSockaddr(sa);
    return {static_cast<std::size_t>(recvd), UdpError::None};
#else
    socklen_t sl  = sizeof(sa);
    ssize_t recvd = ::recvfrom(static_cast<int>(fd_), buf, len, 0, reinterpret_cast<sockaddr*>(&sa), &sl);
    if (recvd < 0)
        return {0, mapErr(errno)};
    src = fromSockaddr(sa);
    return {static_cast<std::size_t>(recvd), UdpError::None};
#endif
}

IpEndpoint UdpSocket::localEndpoint() const
{
    if (fd_ == -1)
        return IpEndpoint{{0, 0, 0, 0}, 0};
    sockaddr_in sa{};
#ifdef _WIN32
    SOCKET s = static_cast<SOCKET>(fd_);
    int sl   = sizeof(sa);
    if (::getsockname(s, reinterpret_cast<sockaddr*>(&sa), &sl) != 0)
        return IpEndpoint{{0, 0, 0, 0}, 0};
    return fromSockaddr(sa);
#else
    socklen_t sl = sizeof(sa);
    if (::getsockname(static_cast<int>(fd_), reinterpret_cast<sockaddr*>(&sa), &sl) != 0)
        return IpEndpoint{{0, 0, 0, 0}, 0};
    return fromSockaddr(sa);
#endif
}
