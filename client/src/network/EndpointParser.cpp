#include "network/EndpointParser.hpp"

#include "Logger.hpp"

#include <sstream>
#include <vector>

IpEndpoint parseEndpoint(const std::string& ip, const std::string& port)
{
    std::uint16_t portNum = 50010;
    try {
        portNum = static_cast<std::uint16_t>(std::stoi(port));
    } catch (...) {
        Logger::instance().warn("Invalid port, using 50010");
    }

    std::vector<int> octets;
    std::istringstream iss(ip);
    std::string part;

    while (std::getline(iss, part, '.')) {
        try {
            int val = std::stoi(part);
            if (val >= 0 && val <= 255)
                octets.push_back(val);
        } catch (...) {
            break;
        }
    }

    if (octets.size() == 4) {
        return IpEndpoint::v4(static_cast<std::uint8_t>(octets[0]), static_cast<std::uint8_t>(octets[1]),
                              static_cast<std::uint8_t>(octets[2]), static_cast<std::uint8_t>(octets[3]), portNum);
    }

    Logger::instance().warn("Invalid IP, using 127.0.0.1");
    return IpEndpoint::v4(127, 0, 0, 1, portNum);
}
