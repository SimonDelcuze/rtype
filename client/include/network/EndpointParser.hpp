#pragma once

#include "network/UdpSocket.hpp"

#include <string>

IpEndpoint parseEndpoint(const std::string& ip, const std::string& port);
