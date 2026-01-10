#pragma once

#include <algorithm>
#include <string>

namespace rtype::utils
{

    inline bool isSafeChatMessage(const std::string& message)
    {
        if (message.empty())
            return true;

        const std::string unsafeChars = "<>&\"'";

        for (char c : message) {
            if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
                return false;
            }

            if (unsafeChars.find(c) != std::string::npos) {
                return false;
            }
        }

        return true;
    }

    inline std::string sanitizeChatMessage(std::string message)
    {
        message.erase(std::remove_if(message.begin(), message.end(),
                                     [](unsigned char c) {
                                         if (c < 32 || c > 126)
                                             return true;
                                         if (c == '<' || c == '>' || c == '&' || c == '"' || c == '\'')
                                             return true;
                                         return false;
                                     }),
                      message.end());

        return message;
    }

} // namespace rtype::utils
