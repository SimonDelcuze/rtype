#include "network/UdpSocket.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
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

    static UdpError mapErrno(int e)
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
} // namespace

UdpSocket::UdpSocket() : fd_(-1) {}

UdpSocket::~UdpSocket()
{
    close();
}

bool UdpSocket::open(const IpEndpoint& bindTo)
{
    if (fd_ != -1)
        close();
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return false;
    int yes = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in sa = toSockaddr(bindTo);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
        ::close(fd);
        return false;
    }
    fd_ = fd;
    setNonBlocking(true);
    return true;
}

void UdpSocket::close()
{
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool UdpSocket::isOpen() const
{
    return fd_ != -1;
}

bool UdpSocket::setNonBlocking(bool enable)
{
    if (fd_ == -1)
        return false;
    int flags = ::fcntl(fd_, F_GETFL, 0);
    if (flags == -1)
        return false;
    if (enable)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return ::fcntl(fd_, F_SETFL, flags) == 0;
}

bool UdpSocket::setRecvBuffer(int bytes)
{
    if (fd_ == -1)
        return false;
    return ::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes)) == 0;
}

bool UdpSocket::setSendBuffer(int bytes)
{
    if (fd_ == -1)
        return false;
    return ::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &bytes, sizeof(bytes)) == 0;
}

UdpResult UdpSocket::sendTo(const std::uint8_t* data, std::size_t len, const IpEndpoint& dst)
{
    if (fd_ == -1)
        return {0, UdpError::NotOpen};
    sockaddr_in sa = toSockaddr(dst);
    ssize_t sent   = ::sendto(fd_, data, len, 0, reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
    if (sent < 0)
        return {0, mapErrno(errno)};
    return {static_cast<std::size_t>(sent), UdpError::None};
}

UdpResult UdpSocket::recvFrom(std::uint8_t* buf, std::size_t len, IpEndpoint& src)
{
    if (fd_ == -1)
        return {0, UdpError::NotOpen};
    sockaddr_in sa{};
    socklen_t sl  = sizeof(sa);
    ssize_t recvd = ::recvfrom(fd_, buf, len, 0, reinterpret_cast<sockaddr*>(&sa), &sl);
    if (recvd < 0)
        return {0, mapErrno(errno)};
    src = fromSockaddr(sa);
    return {static_cast<std::size_t>(recvd), UdpError::None};
}

IpEndpoint UdpSocket::localEndpoint() const
{
    if (fd_ == -1)
        return IpEndpoint{{0, 0, 0, 0}, 0};
    sockaddr_in sa{};
    socklen_t sl = sizeof(sa);
    if (::getsockname(fd_, reinterpret_cast<sockaddr*>(&sa), &sl) != 0)
        return IpEndpoint{{0, 0, 0, 0}, 0};
    return fromSockaddr(sa);
}
