#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace rtype
{

    class JsonParseError : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    class JsonKeyError : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    class JsonTypeError : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    class Json
    {
      public:
        Json();
        ~Json() = default;

        Json(nlohmann::json internal);

        static Json parse(const std::string& str);
        static Json array();
        static Json object();

        [[nodiscard]] std::string dump(int indent = -1) const;

        [[nodiscard]] bool contains(const std::string& key) const;

        template <typename T> T getValue(const std::string& key) const
        {
            if (!_data.contains(key)) {
                throw JsonKeyError("Key not found: " + key);
            }
            try {
                return _data[key].get<T>();
            } catch (const nlohmann::json::exception& e) {
                throw JsonTypeError("Type error for key '" + key + "': " + std::string(e.what()));
            }
        }

        template <typename T> T get() const
        {
            try {
                return _data.get<T>();
            } catch (const nlohmann::json::exception& e) {
                throw JsonTypeError("Type error: " + std::string(e.what()));
            }
        }

        template <typename T> T getValue(const std::string& key, const T& defaultValue) const
        {
            if (!_data.contains(key)) {
                return defaultValue;
            }
            try {
                return _data[key].get<T>();
            } catch (...) {
                return defaultValue;
            }
        }

        template <typename T> void setValue(const std::string& key, const T& value)
        {
            _data[key] = value;
        }
        void pushBack(const Json& element);
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] bool isArray() const;
        [[nodiscard]] bool isObject() const;
        [[nodiscard]] bool isString() const;
        [[nodiscard]] bool isBoolean() const;
        [[nodiscard]] bool isNumber() const;
        [[nodiscard]] bool isNumberInteger() const;
        [[nodiscard]] bool isNumberUnsigned() const;

        [[nodiscard]] std::vector<std::string> getKeys() const;

        const nlohmann::json& getInternal() const
        {
            return _data;
        }
        nlohmann::json& getInternal()
        {
            return _data;
        }

        Json operator[](const std::string& key) const;
        Json operator[](std::size_t index) const;

      private:
        nlohmann::json _data;
    };

} // namespace rtype
