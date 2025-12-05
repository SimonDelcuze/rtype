#include "server/Session.hpp"

#include <sstream>

std::string endpointKey(const IpEndpoint& ep)
{
    std::ostringstream ss;
    ss << static_cast<int>(ep.addr[0]) << "." << static_cast<int>(ep.addr[1]) << "." << static_cast<int>(ep.addr[2])
       << "." << static_cast<int>(ep.addr[3]) << ":" << ep.port;
    return ss.str();
}
