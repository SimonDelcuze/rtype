#include "json/Json.hpp"

namespace rtype
{

    Json::Json() : _data(nlohmann::json::object()) {}

    Json::Json(nlohmann::json internal) : _data(std::move(internal)) {}

    Json Json::parse(const std::string& str)
    {
        try {
            return Json(nlohmann::json::parse(str));
        } catch (const nlohmann::json::parse_error& e) {
            throw JsonParseError("Failed to parse JSON: " + std::string(e.what()));
        }
    }

    Json Json::array()
    {
        return Json(nlohmann::json::array());
    }

    Json Json::object()
    {
        return Json(nlohmann::json::object());
    }

    std::string Json::dump(int indent) const
    {
        return _data.dump(indent);
    }

    bool Json::contains(const std::string& key) const
    {
        return _data.contains(key);
    }

    void Json::pushBack(const Json& element)
    {
        if (!_data.is_array()) {
            if (_data.is_null()) {
                _data = nlohmann::json::array();
            } else {
                throw JsonTypeError("Cannot pushBack to a non-array JSON object");
            }
        }
        _data.push_back(element.getInternal());
    }

    std::size_t Json::size() const
    {
        return _data.size();
    }

    bool Json::empty() const
    {
        return _data.empty();
    }

    bool Json::isArray() const
    {
        return _data.is_array();
    }

    bool Json::isObject() const
    {
        return _data.is_object();
    }

    bool Json::isString() const
    {
        return _data.is_string();
    }

    bool Json::isBoolean() const
    {
        return _data.is_boolean();
    }

    bool Json::isNumber() const
    {
        return _data.is_number();
    }

    bool Json::isNumberInteger() const
    {
        return _data.is_number_integer();
    }

    bool Json::isNumberUnsigned() const
    {
        return _data.is_number_unsigned();
    }

    std::vector<std::string> Json::getKeys() const
    {
        std::vector<std::string> keys;
        if (_data.is_object()) {
            for (auto it = _data.begin(); it != _data.end(); ++it) {
                keys.push_back(it.key());
            }
        }
        return keys;
    }

    Json Json::operator[](const std::string& key) const
    {
        if (!_data.contains(key)) {
            throw JsonKeyError("Key not found: " + key);
        }
        return Json(_data[key]);
    }

    Json Json::operator[](std::size_t index) const
    {
        if (!_data.is_array()) {
            throw JsonTypeError("Cannot query index on non-array JSON object");
        }
        if (index >= _data.size()) {
            throw std::out_of_range("JSON array index out of range");
        }
        return Json(_data[index]);
    }

} // namespace rtype
