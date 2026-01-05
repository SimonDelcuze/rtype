# JSON Wrapper

## Overview

The R-Type project includes a custom JSON wrapper (`rtype::Json`) that provides a safe, type-checked interface around the `nlohmann::json` library. This wrapper aims to simplify JSON parsing and manipulation while providing clear error messages and better exception handling.

## Location

- **Header**: `shared/include/json/Json.hpp`
- **Implementation**: `shared/src/json/Json.cpp`
- **Tests**: `tests/shared/JsonTests.cpp`

## Purpose

The JSON wrapper abstracts the underlying JSON library implementation and provides:

1. **Type safety**: Explicit type checking with clear error messages
2. **Custom exceptions**: Dedicated exception types for different error scenarios
3. **Simplified API**: Clean, intuitive methods for common JSON operations
4. **Default values**: Easy handling of optional fields with fallback values

## Exception Types

The wrapper defines three custom exception types:

### JsonParseError

Thrown when JSON parsing fails (invalid JSON string).

```cpp
throw JsonParseError("Failed to parse JSON: ...");
```

### JsonKeyError

Thrown when trying to access a non-existent key.

```cpp
throw JsonKeyError("Key not found: " + key);
```

### JsonTypeError

Thrown when type conversion fails or when an operation is performed on the wrong JSON type.

```cpp
throw JsonTypeError("Type error for key '" + key + "': ...");
```

## Core API

### Creating JSON Objects

#### Parse from string

```cpp
std::string jsonStr = R"({"name": "player", "health": 100})";
Json json = Json::parse(jsonStr);
```

#### Create empty object

```cpp
Json obj = Json::object();
```

#### Create empty array

```cpp
Json arr = Json::array();
```

### Reading Values

#### Get value with type checking

```cpp
Json json = Json::parse(jsonStr);
std::string name = json.getValue<std::string>("name");
int health = json.getValue<int>("health");
```

Throws `JsonKeyError` if key doesn't exist, or `JsonTypeError` if type doesn't match.

#### Get value with default fallback

```cpp
int score = json.getValue<int>("score", 0);  // Returns 0 if key doesn't exist
std::string team = json.getValue<std::string>("team", "red");
```

#### Get value directly (for non-object JSON)

```cpp
Json numberJson = Json::parse("42");
int value = numberJson.get<int>();
```

#### Check if key exists

```cpp
if (json.contains("powerup")) {
    // Handle powerup
}
```

### Setting Values

```cpp
Json player = Json::object();
player.setValue("name", "Player1");
player.setValue("score", 1000);
player.setValue("active", true);
```

### Array Operations

#### Create and populate array

```cpp
Json enemies = Json::array();

Json enemy1 = Json::object();
enemy1.setValue("type", "zombie");
enemy1.setValue("health", 50);

Json enemy2 = Json::object();
enemy2.setValue("type", "skeleton");
enemy2.setValue("health", 30);

enemies.pushBack(enemy1);
enemies.pushBack(enemy2);
```

#### Access array elements

```cpp
Json firstEnemy = enemies[0];
std::string type = firstEnemy.getValue<std::string>("type");
```

#### Array size

```cpp
std::size_t count = enemies.size();
```

### Type Checking

```cpp
if (json.isObject()) { /* ... */ }
if (json.isArray()) { /* ... */ }
if (json.isString()) { /* ... */ }
if (json.isBoolean()) { /* ... */ }
if (json.isNumber()) { /* ... */ }
if (json.isNumberInteger()) { /* ... */ }
if (json.isNumberUnsigned()) { /* ... */ }
```

### Object Inspection

#### Get all keys

```cpp
std::vector<std::string> keys = json.getKeys();
for (const auto& key : keys) {
    std::cout << key << std::endl;
}
```

#### Check if empty

```cpp
if (json.empty()) {
    // Handle empty JSON
}
```

### Nested Access

```cpp
std::string jsonStr = R"({"config": {"graphics": {"resolution": "1920x1080"}}})";
Json json = Json::parse(jsonStr);

Json config = json["config"];
Json graphics = config["graphics"];
std::string resolution = graphics.getValue<std::string>("resolution");
```

### Serialization

#### Compact output

```cpp
std::string compact = json.dump();  // Single line, no indentation
```

#### Pretty-printed output

```cpp
std::string pretty = json.dump(2);  // Indented with 2 spaces
std::string pretty4 = json.dump(4); // Indented with 4 spaces
```

## Advanced Usage

### Access underlying nlohmann::json

If you need direct access to the underlying `nlohmann::json` object:

```cpp
const nlohmann::json& internal = json.getInternal();
nlohmann::json& mutableInternal = json.getInternal();
```

This is useful when integrating with code that still uses `nlohmann::json` directly.

## Usage Examples

### Parsing a configuration file

```cpp
try {
    std::ifstream file("config.json");
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    Json config = Json::parse(content);

    std::string serverIP = config.getValue<std::string>("server_ip", "localhost");
    int port = config.getValue<int>("port", 8080);
    bool debugMode = config.getValue<bool>("debug", false);

    // Use configuration...
} catch (const JsonParseError& e) {
    std::cerr << "Config parse error: " << e.what() << std::endl;
} catch (const JsonTypeError& e) {
    std::cerr << "Config type error: " << e.what() << std::endl;
}
```

### Building a JSON response

```cpp
Json response = Json::object();
response.setValue("status", "success");
response.setValue("timestamp", std::time(nullptr));

Json data = Json::object();
data.setValue("playerId", 42);
data.setValue("score", 15000);

response.setValue("data", data.getInternal());

std::string jsonString = response.dump(2);
// Send jsonString over network...
```

### Processing a list of entities

```cpp
std::string entitiesJson = R"({
    "entities": [
        {"id": 1, "type": "player", "x": 100, "y": 200},
        {"id": 2, "type": "enemy", "x": 300, "y": 400}
    ]
})";

Json root = Json::parse(entitiesJson);
Json entities = root["entities"];

for (std::size_t i = 0; i < entities.size(); ++i) {
    Json entity = entities[i];

    int id = entity.getValue<int>("id");
    std::string type = entity.getValue<std::string>("type");
    int x = entity.getValue<int>("x");
    int y = entity.getValue<int>("y");

    // Process entity...
}
```

## Migration Guide

If you have code using `nlohmann::json` directly, here's how to migrate:

### Before (nlohmann::json)

```cpp
nlohmann::json json = nlohmann::json::parse(str);
std::string value = json["key"];
int number = json.value("number", 0);
```

### After (rtype::Json)

```cpp
Json json = Json::parse(str);
std::string value = json.getValue<std::string>("key");
int number = json.getValue<int>("number", 0);
```

## Error Handling Best Practices

Always wrap JSON operations in try-catch blocks:

```cpp
try {
    Json config = Json::parse(configString);
    // Process config...
} catch (const JsonParseError& e) {
    // Handle parse errors (invalid JSON)
    std::cerr << "Parse error: " << e.what() << std::endl;
} catch (const JsonKeyError& e) {
    // Handle missing keys
    std::cerr << "Missing key: " << e.what() << std::endl;
} catch (const JsonTypeError& e) {
    // Handle type mismatches
    std::cerr << "Type error: " << e.what() << std::endl;
}
```

## Testing

The JSON wrapper includes comprehensive unit tests in `tests/shared/JsonTests.cpp`. Run these tests to verify the wrapper's behavior:

```bash
# Build and run tests
cmake --build build --target JsonTests
./build/tests/JsonTests
```

## Implementation Notes

- The wrapper uses `nlohmann::json` internally for actual JSON processing
- All methods are designed to be exception-safe
- The `[[nodiscard]]` attribute is used on query methods to prevent ignoring return values
- Default constructors create empty objects (not null)
- Array indexing is bounds-checked and throws `std::out_of_range` for invalid indices
