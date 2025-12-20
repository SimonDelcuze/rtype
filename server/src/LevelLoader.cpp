#include "server/LevelLoader.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <unordered_set>

namespace
{
    using json = nlohmann::json;

    std::string joinPath(const std::string& base, const std::string& token)
    {
        if (base.empty())
            return "/" + token;
        return base + "/" + token;
    }

    void setError(LevelLoadError& error, LevelLoadErrorCode code, const std::string& message, const std::string& path,
                  const std::string& jsonPointer)
    {
        error.code    = code;
        error.message = message;
        if (!path.empty())
            error.path = path;
        error.jsonPointer = jsonPointer;
    }

    bool readFile(const std::string& path, std::string& out, LevelLoadError& error)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            setError(error, LevelLoadErrorCode::FileNotFound, "File not found", path, "");
            return false;
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        if (!file.good() && !file.eof()) {
            setError(error, LevelLoadErrorCode::FileReadError, "Failed to read file", path, "");
            return false;
        }
        out = buffer.str();
        return true;
    }

    bool parseJson(const std::string& text, json& out, LevelLoadError& error, const std::string& path)
    {
        try {
            out = json::parse(text);
            return true;
        } catch (const json::parse_error& e) {
            setError(error, LevelLoadErrorCode::JsonParseError, e.what(), path, "");
            return false;
        }
    }

    bool requireObject(const json& obj, const std::string& key, const json*& out, const std::string& path,
                       LevelLoadError& error)
    {
        if (!obj.contains(key)) {
            setError(error, LevelLoadErrorCode::SchemaError, "Missing object: " + key, "", joinPath(path, key));
            return false;
        }
        const json& value = obj.at(key);
        if (!value.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected object: " + key, "", joinPath(path, key));
            return false;
        }
        out = &value;
        return true;
    }

    bool requireArray(const json& obj, const std::string& key, const json*& out, const std::string& path,
                      LevelLoadError& error)
    {
        if (!obj.contains(key)) {
            setError(error, LevelLoadErrorCode::SchemaError, "Missing array: " + key, "", joinPath(path, key));
            return false;
        }
        const json& value = obj.at(key);
        if (!value.is_array()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected array: " + key, "", joinPath(path, key));
            return false;
        }
        out = &value;
        return true;
    }

    bool readString(const json& obj, const std::string& key, std::string& out, const std::string& path,
                    LevelLoadError& error, bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing string: " + key, "", joinPath(path, key));
            return false;
        }
        const json& value = obj.at(key);
        if (!value.is_string()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected string: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<std::string>();
        return true;
    }

    bool readBool(const json& obj, const std::string& key, bool& out, const std::string& path, LevelLoadError& error,
                  bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing bool: " + key, "", joinPath(path, key));
            return false;
        }
        const json& value = obj.at(key);
        if (!value.is_boolean()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected bool: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<bool>();
        return true;
    }

    bool readNumber(const json& obj, const std::string& key, double& out, const std::string& path,
                    LevelLoadError& error, bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing number: " + key, "", joinPath(path, key));
            return false;
        }
        const json& value = obj.at(key);
        if (!value.is_number()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected number: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<double>();
        return true;
    }

    bool readInt(const json& obj, const std::string& key, std::int32_t& out, const std::string& path,
                 LevelLoadError& error, bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing integer: " + key, "", joinPath(path, key));
            return false;
        }
        const json& value = obj.at(key);
        if (!value.is_number_integer() && !value.is_number_unsigned()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected integer: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<std::int32_t>();
        return true;
    }

    bool parseVec2(const json& j, Vec2f& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_array() || j.size() != 2) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected [x,y] array", "", path);
            return false;
        }
        if (!j[0].is_number() || !j[1].is_number()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected numeric vector2", "", path);
            return false;
        }
        out.x = j[0].get<float>();
        out.y = j[1].get<float>();
        return true;
    }

    bool finiteVec(const Vec2f& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y);
    }

    bool parseHitbox(const json& j, HitboxComponent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected hitbox object", "", path);
            return false;
        }
        double width   = 0.0;
        double height  = 0.0;
        double offsetX = 0.0;
        double offsetY = 0.0;
        bool active    = true;
        if (!readNumber(j, "width", width, path, error, true))
            return false;
        if (!readNumber(j, "height", height, path, error, true))
            return false;
        readNumber(j, "offsetX", offsetX, path, error, false);
        readNumber(j, "offsetY", offsetY, path, error, false);
        readBool(j, "active", active, path, error, false);
        out = HitboxComponent::create(static_cast<float>(width), static_cast<float>(height),
                                      static_cast<float>(offsetX), static_cast<float>(offsetY), active);
        return true;
    }

    bool parseCollider(const json& j, ColliderComponent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected collider object", "", path);
            return false;
        }
        std::string shape;
        if (!readString(j, "shape", shape, path, error, true))
            return false;
        double offsetX = 0.0;
        double offsetY = 0.0;
        bool active    = true;
        readNumber(j, "offsetX", offsetX, path, error, false);
        readNumber(j, "offsetY", offsetY, path, error, false);
        readBool(j, "active", active, path, error, false);
        if (shape == "box") {
            double width  = 0.0;
            double height = 0.0;
            if (!readNumber(j, "width", width, path, error, true))
                return false;
            if (!readNumber(j, "height", height, path, error, true))
                return false;
            out = ColliderComponent::box(static_cast<float>(width), static_cast<float>(height),
                                         static_cast<float>(offsetX), static_cast<float>(offsetY), active);
            return true;
        }
        if (shape == "circle") {
            double radius = 0.0;
            if (!readNumber(j, "radius", radius, path, error, true))
                return false;
            out = ColliderComponent::circle(static_cast<float>(radius), static_cast<float>(offsetX),
                                            static_cast<float>(offsetY), active);
            return true;
        }
        if (shape == "polygon") {
            if (!j.contains("points") || !j.at("points").is_array()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing polygon points", "",
                         joinPath(path, "points"));
                return false;
            }
            const json& pts = j.at("points");
            if (pts.size() < 3) {
                setError(error, LevelLoadErrorCode::SchemaError, "Polygon needs at least 3 points", "", path);
                return false;
            }
            std::vector<std::array<float, 2>> points;
            points.reserve(pts.size());
            for (std::size_t i = 0; i < pts.size(); ++i) {
                const json& p = pts[i];
                Vec2f v{};
                if (!parseVec2(p, v, joinPath(joinPath(path, "points"), std::to_string(i)), error))
                    return false;
                points.push_back({v.x, v.y});
            }
            out = ColliderComponent::polygon(points, static_cast<float>(offsetX), static_cast<float>(offsetY), active);
            return true;
        }
        setError(error, LevelLoadErrorCode::SchemaError, "Unknown collider shape: " + shape, "", path);
        return false;
    }

    bool parseShooting(const json& j, EnemyShootingComponent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected shooting object", "", path);
            return false;
        }
        double interval     = 0.0;
        double speed        = 0.0;
        std::int32_t damage = 0;
        double lifetime     = 0.0;
        if (!readNumber(j, "interval", interval, path, error, true))
            return false;
        if (!readNumber(j, "speed", speed, path, error, true))
            return false;
        if (!readInt(j, "damage", damage, path, error, true))
            return false;
        if (!readNumber(j, "lifetime", lifetime, path, error, true))
            return false;
        out = EnemyShootingComponent::create(static_cast<float>(interval), static_cast<float>(speed), damage,
                                             static_cast<float>(lifetime));
        return true;
    }

    bool parseScroll(const json& j, ScrollSettings& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected scroll object", "", path);
            return false;
        }
        std::string mode;
        if (!readString(j, "mode", mode, path, error, true))
            return false;
        if (mode == "constant") {
            double speed = 0.0;
            if (!readNumber(j, "speedX", speed, path, error, true))
                return false;
            out.mode   = ScrollMode::Constant;
            out.speedX = static_cast<float>(speed);
            return true;
        }
        if (mode == "stopped") {
            out.mode   = ScrollMode::Stopped;
            out.speedX = 0.0F;
            return true;
        }
        if (mode == "curve") {
            out.mode = ScrollMode::Curve;
            if (!j.contains("curve") || !j.at("curve").is_array()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scroll curve", "", joinPath(path, "curve"));
                return false;
            }
            const json& curve = j.at("curve");
            out.curve.clear();
            out.curve.reserve(curve.size());
            for (std::size_t i = 0; i < curve.size(); ++i) {
                const json& k = curve[i];
                if (!k.is_object()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid curve keyframe", "",
                             joinPath(joinPath(path, "curve"), std::to_string(i)));
                    return false;
                }
                double time    = 0.0;
                double speedX  = 0.0;
                std::string kp = joinPath(joinPath(path, "curve"), std::to_string(i));
                if (!readNumber(k, "time", time, kp, error, true))
                    return false;
                if (!readNumber(k, "speedX", speedX, kp, error, true))
                    return false;
                out.curve.push_back(ScrollKeyframe{static_cast<float>(time), static_cast<float>(speedX)});
            }
            return true;
        }
        setError(error, LevelLoadErrorCode::SchemaError, "Unknown scroll mode: " + mode, "", path);
        return false;
    }

    bool parseTrigger(const json& j, Trigger& out, const std::string& path, LevelLoadError& error);

    bool parseRepeat(const json& j, RepeatSpec& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected repeat object", "", path);
            return false;
        }
        double interval = 0.0;
        if (!readNumber(j, "interval", interval, path, error, true))
            return false;
        out.interval = static_cast<float>(interval);
        if (j.contains("count")) {
            std::int32_t count = 0;
            if (!readInt(j, "count", count, path, error, true))
                return false;
            out.count = count;
        }
        if (j.contains("until")) {
            Trigger until;
            if (!parseTrigger(j.at("until"), until, joinPath(path, "until"), error))
                return false;
            out.until = until;
        }
        return true;
    }

    bool parseTrigger(const json& j, Trigger& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected trigger object", "", path);
            return false;
        }
        std::string type;
        if (!readString(j, "type", type, path, error, true))
            return false;
        if (type == "time") {
            double time = 0.0;
            if (!readNumber(j, "time", time, path, error, true))
                return false;
            out.type = TriggerType::Time;
            out.time = static_cast<float>(time);
            return true;
        }
        if (type == "distance") {
            double distance = 0.0;
            if (!readNumber(j, "distance", distance, path, error, true))
                return false;
            out.type     = TriggerType::Distance;
            out.distance = static_cast<float>(distance);
            return true;
        }
        if (type == "spawn_dead") {
            std::string spawnId;
            if (!readString(j, "spawnId", spawnId, path, error, true))
                return false;
            out.type    = TriggerType::SpawnDead;
            out.spawnId = spawnId;
            return true;
        }
        if (type == "boss_dead") {
            std::string bossId;
            if (!readString(j, "bossId", bossId, path, error, true))
                return false;
            out.type   = TriggerType::BossDead;
            out.bossId = bossId;
            return true;
        }
        if (type == "enemy_count_at_most") {
            std::int32_t count = 0;
            if (!readInt(j, "count", count, path, error, true))
                return false;
            out.type  = TriggerType::EnemyCountAtMost;
            out.count = count;
            return true;
        }
        if (type == "checkpoint_reached") {
            std::string checkpointId;
            if (!readString(j, "checkpointId", checkpointId, path, error, true))
                return false;
            out.type         = TriggerType::CheckpointReached;
            out.checkpointId = checkpointId;
            return true;
        }
        if (type == "hp_below") {
            std::string bossId;
            std::int32_t value = 0;
            if (!readString(j, "bossId", bossId, path, error, true))
                return false;
            if (!readInt(j, "value", value, path, error, true))
                return false;
            out.type   = TriggerType::HpBelow;
            out.bossId = bossId;
            out.value  = value;
            return true;
        }
        if (type == "all_of" || type == "any_of") {
            if (!j.contains("triggers") || !j.at("triggers").is_array()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing triggers array", "",
                         joinPath(path, "triggers"));
                return false;
            }
            out.type        = type == "all_of" ? TriggerType::AllOf : TriggerType::AnyOf;
            const json& arr = j.at("triggers");
            out.triggers.clear();
            out.triggers.reserve(arr.size());
            for (std::size_t i = 0; i < arr.size(); ++i) {
                Trigger child;
                if (!parseTrigger(arr[i], child, joinPath(joinPath(path, "triggers"), std::to_string(i)), error))
                    return false;
                out.triggers.push_back(std::move(child));
            }
            return true;
        }
        setError(error, LevelLoadErrorCode::SchemaError, "Unknown trigger type: " + type, "", path);
        return false;
    }

    bool parseWave(const json& j, WaveDefinition& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected wave object", "", path);
            return false;
        }
        std::string type;
        if (!readString(j, "type", type, path, error, true))
            return false;
        if (!readString(j, "enemy", out.enemy, path, error, true))
            return false;
        if (!readString(j, "patternId", out.patternId, path, error, true))
            return false;

        if (j.contains("health")) {
            std::int32_t h = 0;
            if (!readInt(j, "health", h, path, error, true))
                return false;
            out.health = h;
        }
        if (j.contains("scale")) {
            Vec2f scale{};
            if (!parseVec2(j.at("scale"), scale, joinPath(path, "scale"), error))
                return false;
            out.scale = scale;
        }
        if (j.contains("shootingEnabled")) {
            bool shootingEnabled = true;
            if (!readBool(j, "shootingEnabled", shootingEnabled, path, error, true))
                return false;
            out.shootingEnabled = shootingEnabled;
        }

        if (type == "line") {
            out.type           = WaveType::Line;
            double spawnX      = 0.0;
            double startY      = 0.0;
            double deltaY      = 0.0;
            std::int32_t count = 0;
            if (!readNumber(j, "spawnX", spawnX, path, error, true))
                return false;
            if (!readNumber(j, "startY", startY, path, error, true))
                return false;
            if (!readNumber(j, "deltaY", deltaY, path, error, true))
                return false;
            if (!readInt(j, "count", count, path, error, true))
                return false;
            out.spawnX = static_cast<float>(spawnX);
            out.startY = static_cast<float>(startY);
            out.deltaY = static_cast<float>(deltaY);
            out.count  = count;
            return true;
        }
        if (type == "stagger") {
            out.type           = WaveType::Stagger;
            double spawnX      = 0.0;
            double startY      = 0.0;
            double deltaY      = 0.0;
            double spacing     = 0.0;
            std::int32_t count = 0;
            if (!readNumber(j, "spawnX", spawnX, path, error, true))
                return false;
            if (!readNumber(j, "startY", startY, path, error, true))
                return false;
            if (!readNumber(j, "deltaY", deltaY, path, error, true))
                return false;
            if (!readNumber(j, "spacing", spacing, path, error, true))
                return false;
            if (!readInt(j, "count", count, path, error, true))
                return false;
            out.spawnX  = static_cast<float>(spawnX);
            out.startY  = static_cast<float>(startY);
            out.deltaY  = static_cast<float>(deltaY);
            out.spacing = static_cast<float>(spacing);
            out.count   = count;
            return true;
        }
        if (type == "triangle") {
            out.type              = WaveType::Triangle;
            double spawnX         = 0.0;
            double apexY          = 0.0;
            double rowHeight      = 0.0;
            double horizontalStep = 0.0;
            std::int32_t layers   = 0;
            if (!readNumber(j, "spawnX", spawnX, path, error, true))
                return false;
            if (!readNumber(j, "apexY", apexY, path, error, true))
                return false;
            if (!readNumber(j, "rowHeight", rowHeight, path, error, true))
                return false;
            if (!readNumber(j, "horizontalStep", horizontalStep, path, error, true))
                return false;
            if (!readInt(j, "layers", layers, path, error, true))
                return false;
            out.spawnX         = static_cast<float>(spawnX);
            out.apexY          = static_cast<float>(apexY);
            out.rowHeight      = static_cast<float>(rowHeight);
            out.horizontalStep = static_cast<float>(horizontalStep);
            out.layers         = layers;
            return true;
        }
        if (type == "serpent") {
            out.type           = WaveType::Serpent;
            double spawnX      = 0.0;
            double startY      = 0.0;
            double stepY       = 0.0;
            double amplitudeX  = 0.0;
            double stepTime    = 0.0;
            std::int32_t count = 0;
            if (!readNumber(j, "spawnX", spawnX, path, error, true))
                return false;
            if (!readNumber(j, "startY", startY, path, error, true))
                return false;
            if (!readNumber(j, "stepY", stepY, path, error, true))
                return false;
            if (!readNumber(j, "amplitudeX", amplitudeX, path, error, true))
                return false;
            if (!readNumber(j, "stepTime", stepTime, path, error, true))
                return false;
            if (!readInt(j, "count", count, path, error, true))
                return false;
            out.spawnX     = static_cast<float>(spawnX);
            out.startY     = static_cast<float>(startY);
            out.stepY      = static_cast<float>(stepY);
            out.amplitudeX = static_cast<float>(amplitudeX);
            out.stepTime   = static_cast<float>(stepTime);
            out.count      = count;
            return true;
        }
        if (type == "cross") {
            out.type               = WaveType::Cross;
            double centerX         = 0.0;
            double centerY         = 0.0;
            double step            = 0.0;
            std::int32_t armLength = 0;
            if (!readNumber(j, "centerX", centerX, path, error, true))
                return false;
            if (!readNumber(j, "centerY", centerY, path, error, true))
                return false;
            if (!readNumber(j, "step", step, path, error, true))
                return false;
            if (!readInt(j, "armLength", armLength, path, error, true))
                return false;
            out.centerX   = static_cast<float>(centerX);
            out.centerY   = static_cast<float>(centerY);
            out.step      = static_cast<float>(step);
            out.armLength = armLength;
            return true;
        }
        setError(error, LevelLoadErrorCode::SchemaError, "Unknown wave type: " + type, "", path);
        return false;
    }

    bool parseBounds(const json& j, CameraBounds& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected bounds object", "", path);
            return false;
        }
        double minX = 0.0;
        double maxX = 0.0;
        double minY = 0.0;
        double maxY = 0.0;
        if (!readNumber(j, "minX", minX, path, error, true))
            return false;
        if (!readNumber(j, "maxX", maxX, path, error, true))
            return false;
        if (!readNumber(j, "minY", minY, path, error, true))
            return false;
        if (!readNumber(j, "maxY", maxY, path, error, true))
            return false;
        out.minX = static_cast<float>(minX);
        out.maxX = static_cast<float>(maxX);
        out.minY = static_cast<float>(minY);
        out.maxY = static_cast<float>(maxY);
        return true;
    }

    bool parseEvent(const json& j, LevelEvent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected event object", "", path);
            return false;
        }
        std::string type;
        if (!readString(j, "type", type, path, error, true))
            return false;
        readString(j, "id", out.id, path, error, false);
        if (!j.contains("trigger")) {
            setError(error, LevelLoadErrorCode::SchemaError, "Missing trigger", "", joinPath(path, "trigger"));
            return false;
        }
        if (!parseTrigger(j.at("trigger"), out.trigger, joinPath(path, "trigger"), error))
            return false;
        if (j.contains("repeat")) {
            RepeatSpec repeat;
            if (!parseRepeat(j.at("repeat"), repeat, joinPath(path, "repeat"), error))
                return false;
            out.repeat = repeat;
        }
        if (type == "spawn_wave") {
            out.type = EventType::SpawnWave;
            if (!j.contains("wave")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing wave", "", joinPath(path, "wave"));
                return false;
            }
            WaveDefinition wave;
            if (!parseWave(j.at("wave"), wave, joinPath(path, "wave"), error))
                return false;
            out.wave = wave;
            return true;
        }
        if (type == "spawn_obstacle") {
            out.type = EventType::SpawnObstacle;
            SpawnObstacleSettings settings;
            if (!readString(j, "obstacle", settings.obstacle, path, error, true))
                return false;
            readString(j, "spawnId", settings.spawnId, path, error, false);
            double x = 0.0;
            if (!readNumber(j, "x", x, path, error, true))
                return false;
            settings.x = static_cast<float>(x);
            if (j.contains("y")) {
                double y = 0.0;
                if (!readNumber(j, "y", y, path, error, true))
                    return false;
                settings.y = static_cast<float>(y);
            }
            if (j.contains("anchor")) {
                std::string anchor;
                if (!readString(j, "anchor", anchor, path, error, true))
                    return false;
                if (anchor == "top")
                    settings.anchor = ObstacleAnchor::Top;
                else if (anchor == "bottom")
                    settings.anchor = ObstacleAnchor::Bottom;
                else if (anchor == "absolute")
                    settings.anchor = ObstacleAnchor::Absolute;
                else {
                    setError(error, LevelLoadErrorCode::SchemaError, "Unknown anchor: " + anchor, "",
                             joinPath(path, "anchor"));
                    return false;
                }
            }
            if (j.contains("margin")) {
                double margin = 0.0;
                if (!readNumber(j, "margin", margin, path, error, true))
                    return false;
                settings.margin = static_cast<float>(margin);
            }
            if (j.contains("health")) {
                std::int32_t health = 0;
                if (!readInt(j, "health", health, path, error, true))
                    return false;
                settings.health = health;
            }
            if (j.contains("scale")) {
                Vec2f scale{};
                if (!parseVec2(j.at("scale"), scale, joinPath(path, "scale"), error))
                    return false;
                settings.scale = scale;
            }
            if (j.contains("speedX")) {
                double speedX = 0.0;
                if (!readNumber(j, "speedX", speedX, path, error, true))
                    return false;
                settings.speedX = static_cast<float>(speedX);
            }
            if (j.contains("speedY")) {
                double speedY = 0.0;
                if (!readNumber(j, "speedY", speedY, path, error, true))
                    return false;
                settings.speedY = static_cast<float>(speedY);
            }
            out.obstacle = settings;
            return true;
        }
        if (type == "spawn_boss") {
            out.type = EventType::SpawnBoss;
            SpawnBossSettings settings;
            if (!readString(j, "bossId", settings.bossId, path, error, true))
                return false;
            readString(j, "spawnId", settings.spawnId, path, error, false);
            if (!j.contains("spawn") || !j.at("spawn").is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing spawn", "", joinPath(path, "spawn"));
                return false;
            }
            const json& spawn = j.at("spawn");
            double x          = 0.0;
            double y          = 0.0;
            if (!readNumber(spawn, "x", x, joinPath(path, "spawn"), error, true))
                return false;
            if (!readNumber(spawn, "y", y, joinPath(path, "spawn"), error, true))
                return false;
            settings.spawn = Vec2f{static_cast<float>(x), static_cast<float>(y)};
            out.boss       = settings;
            return true;
        }
        if (type == "set_scroll") {
            out.type = EventType::SetScroll;
            ScrollSettings scroll;
            if (!j.contains("scroll")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scroll", "", joinPath(path, "scroll"));
                return false;
            }
            if (!parseScroll(j.at("scroll"), scroll, joinPath(path, "scroll"), error))
                return false;
            out.scroll = scroll;
            return true;
        }
        if (type == "set_background") {
            out.type = EventType::SetBackground;
            std::string id;
            if (!readString(j, "backgroundId", id, path, error, true))
                return false;
            out.backgroundId = id;
            return true;
        }
        if (type == "set_music") {
            out.type = EventType::SetMusic;
            std::string id;
            if (!readString(j, "musicId", id, path, error, true))
                return false;
            out.musicId = id;
            return true;
        }
        if (type == "set_camera_bounds") {
            out.type = EventType::SetCameraBounds;
            if (!j.contains("bounds")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing bounds", "", joinPath(path, "bounds"));
                return false;
            }
            CameraBounds bounds;
            if (!parseBounds(j.at("bounds"), bounds, joinPath(path, "bounds"), error))
                return false;
            out.cameraBounds = bounds;
            return true;
        }
        if (type == "gate_open") {
            out.type = EventType::GateOpen;
            std::string gateId;
            if (!readString(j, "gateId", gateId, path, error, true))
                return false;
            out.gateId = gateId;
            return true;
        }
        if (type == "gate_close") {
            out.type = EventType::GateClose;
            std::string gateId;
            if (!readString(j, "gateId", gateId, path, error, true))
                return false;
            out.gateId = gateId;
            return true;
        }
        if (type == "checkpoint") {
            out.type = EventType::Checkpoint;
            CheckpointDefinition checkpoint;
            if (!readString(j, "checkpointId", checkpoint.checkpointId, path, error, true))
                return false;
            if (!j.contains("respawn") || !j.at("respawn").is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing respawn", "", joinPath(path, "respawn"));
                return false;
            }
            const json& respawn = j.at("respawn");
            double x            = 0.0;
            double y            = 0.0;
            if (!readNumber(respawn, "x", x, joinPath(path, "respawn"), error, true))
                return false;
            if (!readNumber(respawn, "y", y, joinPath(path, "respawn"), error, true))
                return false;
            checkpoint.respawn = Vec2f{static_cast<float>(x), static_cast<float>(y)};
            out.checkpoint     = checkpoint;
            return true;
        }
        setError(error, LevelLoadErrorCode::SchemaError, "Unknown event type: " + type, "", path);
        return false;
    }

    bool parsePatterns(const json& j, std::vector<PatternDefinition>& out, const std::string& path,
                       LevelLoadError& error)
    {
        if (!j.is_array()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected patterns array", "", path);
            return false;
        }
        out.clear();
        out.reserve(j.size());
        for (std::size_t i = 0; i < j.size(); ++i) {
            const json& p     = j[i];
            std::string ppath = joinPath(path, std::to_string(i));
            if (!p.is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid pattern object", "", ppath);
                return false;
            }
            std::string id;
            std::string type;
            if (!readString(p, "id", id, ppath, error, true))
                return false;
            if (!readString(p, "type", type, ppath, error, true))
                return false;
            if (type == "linear") {
                double speed = 0.0;
                if (!readNumber(p, "speed", speed, ppath, error, true))
                    return false;
                out.push_back(PatternDefinition{id, MovementComponent::linear(static_cast<float>(speed))});
                continue;
            }
            if (type == "zigzag") {
                double speed     = 0.0;
                double amplitude = 0.0;
                double frequency = 0.0;
                if (!readNumber(p, "speed", speed, ppath, error, true))
                    return false;
                if (!readNumber(p, "amplitude", amplitude, ppath, error, true))
                    return false;
                if (!readNumber(p, "frequency", frequency, ppath, error, true))
                    return false;
                out.push_back(PatternDefinition{id, MovementComponent::zigzag(static_cast<float>(speed),
                                                                              static_cast<float>(amplitude),
                                                                              static_cast<float>(frequency))});
                continue;
            }
            if (type == "sine") {
                double speed     = 0.0;
                double amplitude = 0.0;
                double frequency = 0.0;
                double phase     = 0.0;
                if (!readNumber(p, "speed", speed, ppath, error, true))
                    return false;
                if (!readNumber(p, "amplitude", amplitude, ppath, error, true))
                    return false;
                if (!readNumber(p, "frequency", frequency, ppath, error, true))
                    return false;
                readNumber(p, "phase", phase, ppath, error, false);
                out.push_back(PatternDefinition{
                    id, MovementComponent::sine(static_cast<float>(speed), static_cast<float>(amplitude),
                                                static_cast<float>(frequency), static_cast<float>(phase))});
                continue;
            }
            setError(error, LevelLoadErrorCode::SchemaError, "Unknown pattern type: " + type, "", ppath);
            return false;
        }
        return true;
    }

    bool parseTemplates(const json& j, LevelTemplates& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected templates object", "", path);
            return false;
        }
        const json* hitboxes  = nullptr;
        const json* colliders = nullptr;
        const json* enemies   = nullptr;
        const json* obstacles = nullptr;
        if (!requireObject(j, "hitboxes", hitboxes, path, error))
            return false;
        if (!requireObject(j, "colliders", colliders, path, error))
            return false;
        if (!requireObject(j, "enemies", enemies, path, error))
            return false;
        if (!requireObject(j, "obstacles", obstacles, path, error))
            return false;

        out.hitboxes.clear();
        for (auto it = hitboxes->begin(); it != hitboxes->end(); ++it) {
            HitboxComponent hb{};
            std::string hpath = joinPath(joinPath(path, "hitboxes"), it.key());
            if (!parseHitbox(it.value(), hb, hpath, error))
                return false;
            out.hitboxes[it.key()] = hb;
        }

        out.colliders.clear();
        for (auto it = colliders->begin(); it != colliders->end(); ++it) {
            ColliderComponent col{};
            std::string cpath = joinPath(joinPath(path, "colliders"), it.key());
            if (!parseCollider(it.value(), col, cpath, error))
                return false;
            out.colliders[it.key()] = col;
        }

        out.enemies.clear();
        for (auto it = enemies->begin(); it != enemies->end(); ++it) {
            const json& e     = it.value();
            std::string epath = joinPath(joinPath(path, "enemies"), it.key());
            if (!e.is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid enemy template", "", epath);
                return false;
            }
            std::int32_t typeId = 0;
            std::string hitboxId;
            std::string colliderId;
            std::int32_t health = 0;
            if (!readInt(e, "typeId", typeId, epath, error, true))
                return false;
            if (!readString(e, "hitbox", hitboxId, epath, error, true))
                return false;
            if (!readString(e, "collider", colliderId, epath, error, true))
                return false;
            if (!readInt(e, "health", health, epath, error, true))
                return false;
            if (!e.contains("scale")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scale", "", joinPath(epath, "scale"));
                return false;
            }
            Vec2f scale{};
            if (!parseVec2(e.at("scale"), scale, joinPath(epath, "scale"), error))
                return false;
            EnemyTemplate tpl{};
            tpl.typeId = static_cast<std::uint16_t>(typeId);
            auto hbIt  = out.hitboxes.find(hitboxId);
            if (hbIt == out.hitboxes.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown hitbox: " + hitboxId, "", epath);
                return false;
            }
            auto colIt = out.colliders.find(colliderId);
            if (colIt == out.colliders.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown collider: " + colliderId, "", epath);
                return false;
            }
            tpl.hitbox   = hbIt->second;
            tpl.collider = colIt->second;
            tpl.health   = health;
            tpl.scale    = scale;
            if (e.contains("shooting")) {
                EnemyShootingComponent shooting;
                if (!parseShooting(e.at("shooting"), shooting, joinPath(epath, "shooting"), error))
                    return false;
                tpl.shooting = shooting;
            }
            out.enemies[it.key()] = tpl;
        }

        out.obstacles.clear();
        for (auto it = obstacles->begin(); it != obstacles->end(); ++it) {
            const json& o     = it.value();
            std::string opath = joinPath(joinPath(path, "obstacles"), it.key());
            if (!o.is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid obstacle template", "", opath);
                return false;
            }
            std::int32_t typeId = 0;
            std::string hitboxId;
            std::string colliderId;
            std::int32_t health = 0;
            std::string anchor;
            double margin = 0.0;
            double speedX = 0.0;
            double speedY = 0.0;
            if (!readInt(o, "typeId", typeId, opath, error, true))
                return false;
            if (!readString(o, "hitbox", hitboxId, opath, error, true))
                return false;
            if (!readString(o, "collider", colliderId, opath, error, true))
                return false;
            if (!readInt(o, "health", health, opath, error, true))
                return false;
            if (!readString(o, "anchor", anchor, opath, error, true))
                return false;
            if (o.contains("margin"))
                readNumber(o, "margin", margin, opath, error, true);
            if (!readNumber(o, "speedX", speedX, opath, error, true))
                return false;
            if (!readNumber(o, "speedY", speedY, opath, error, true))
                return false;
            if (!o.contains("scale")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scale", "", joinPath(opath, "scale"));
                return false;
            }
            Vec2f scale{};
            if (!parseVec2(o.at("scale"), scale, joinPath(opath, "scale"), error))
                return false;

            ObstacleTemplate tpl{};
            tpl.typeId = static_cast<std::uint16_t>(typeId);
            auto hbIt  = out.hitboxes.find(hitboxId);
            if (hbIt == out.hitboxes.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown hitbox: " + hitboxId, "", opath);
                return false;
            }
            auto colIt = out.colliders.find(colliderId);
            if (colIt == out.colliders.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown collider: " + colliderId, "", opath);
                return false;
            }
            tpl.hitbox   = hbIt->second;
            tpl.collider = colIt->second;
            tpl.health   = health;
            if (anchor == "top")
                tpl.anchor = ObstacleAnchor::Top;
            else if (anchor == "bottom")
                tpl.anchor = ObstacleAnchor::Bottom;
            else if (anchor == "absolute")
                tpl.anchor = ObstacleAnchor::Absolute;
            else {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown anchor: " + anchor, "", opath);
                return false;
            }
            tpl.margin              = static_cast<float>(margin);
            tpl.speedX              = static_cast<float>(speedX);
            tpl.speedY              = static_cast<float>(speedY);
            tpl.scale               = scale;
            out.obstacles[it.key()] = tpl;
        }
        return true;
    }

    bool parseBosses(const json& j, const LevelTemplates& templates,
                     std::unordered_map<std::string, BossDefinition>& out, const std::string& path,
                     LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected bosses object", "", path);
            return false;
        }
        out.clear();
        for (auto it = j.begin(); it != j.end(); ++it) {
            const json& b     = it.value();
            std::string bpath = joinPath(path, it.key());
            if (!b.is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid boss object", "", bpath);
                return false;
            }
            std::int32_t typeId = 0;
            std::string hitboxId;
            std::string colliderId;
            std::int32_t health = 0;
            if (!readInt(b, "typeId", typeId, bpath, error, true))
                return false;
            if (!readString(b, "hitbox", hitboxId, bpath, error, true))
                return false;
            if (!readString(b, "collider", colliderId, bpath, error, true))
                return false;
            if (!readInt(b, "health", health, bpath, error, true))
                return false;
            if (!b.contains("scale")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scale", "", joinPath(bpath, "scale"));
                return false;
            }
            Vec2f scale{};
            if (!parseVec2(b.at("scale"), scale, joinPath(bpath, "scale"), error))
                return false;

            BossDefinition boss{};
            boss.typeId = static_cast<std::uint16_t>(typeId);
            boss.health = health;
            boss.scale  = scale;
            auto hbIt   = templates.hitboxes.find(hitboxId);
            if (hbIt == templates.hitboxes.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown hitbox: " + hitboxId, "", bpath);
                return false;
            }
            auto colIt = templates.colliders.find(colliderId);
            if (colIt == templates.colliders.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown collider: " + colliderId, "", bpath);
                return false;
            }
            boss.hitbox   = hbIt->second;
            boss.collider = colIt->second;

            if (b.contains("phases")) {
                if (!b.at("phases").is_array()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid phases", "", joinPath(bpath, "phases"));
                    return false;
                }
                const json& phases = b.at("phases");
                boss.phases.reserve(phases.size());
                for (std::size_t i = 0; i < phases.size(); ++i) {
                    const json& p     = phases[i];
                    std::string ppath = joinPath(joinPath(bpath, "phases"), std::to_string(i));
                    if (!p.is_object()) {
                        setError(error, LevelLoadErrorCode::SchemaError, "Invalid phase", "", ppath);
                        return false;
                    }
                    BossPhase phase;
                    if (!readString(p, "id", phase.id, ppath, error, true))
                        return false;
                    if (!p.contains("trigger")) {
                        setError(error, LevelLoadErrorCode::SchemaError, "Missing trigger", "",
                                 joinPath(ppath, "trigger"));
                        return false;
                    }
                    if (!parseTrigger(p.at("trigger"), phase.trigger, joinPath(ppath, "trigger"), error))
                        return false;
                    if (!p.contains("events") || !p.at("events").is_array()) {
                        setError(error, LevelLoadErrorCode::SchemaError, "Missing events", "",
                                 joinPath(ppath, "events"));
                        return false;
                    }
                    const json& evs = p.at("events");
                    phase.events.reserve(evs.size());
                    for (std::size_t e = 0; e < evs.size(); ++e) {
                        LevelEvent ev;
                        if (!parseEvent(evs[e], ev, joinPath(joinPath(ppath, "events"), std::to_string(e)), error))
                            return false;
                        phase.events.push_back(std::move(ev));
                    }
                    boss.phases.push_back(std::move(phase));
                }
            }
            if (b.contains("onDeath")) {
                if (!b.at("onDeath").is_array()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid onDeath", "", joinPath(bpath, "onDeath"));
                    return false;
                }
                const json& evs = b.at("onDeath");
                boss.onDeath.reserve(evs.size());
                for (std::size_t e = 0; e < evs.size(); ++e) {
                    LevelEvent ev;
                    if (!parseEvent(evs[e], ev, joinPath(joinPath(bpath, "onDeath"), std::to_string(e)), error))
                        return false;
                    boss.onDeath.push_back(std::move(ev));
                }
            }
            out[it.key()] = std::move(boss);
        }
        return true;
    }

    bool parseSegments(const json& j, std::vector<LevelSegment>& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_array()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected segments array", "", path);
            return false;
        }
        out.clear();
        out.reserve(j.size());
        for (std::size_t i = 0; i < j.size(); ++i) {
            const json& s     = j[i];
            std::string spath = joinPath(path, std::to_string(i));
            if (!s.is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid segment object", "", spath);
                return false;
            }
            LevelSegment seg;
            if (!readString(s, "id", seg.id, spath, error, true))
                return false;
            if (!s.contains("scroll")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scroll", "", joinPath(spath, "scroll"));
                return false;
            }
            if (!parseScroll(s.at("scroll"), seg.scroll, joinPath(spath, "scroll"), error))
                return false;
            if (s.contains("events")) {
                if (!s.at("events").is_array()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid events", "", joinPath(spath, "events"));
                    return false;
                }
                const json& evs = s.at("events");
                seg.events.reserve(evs.size());
                for (std::size_t e = 0; e < evs.size(); ++e) {
                    LevelEvent ev;
                    if (!parseEvent(evs[e], ev, joinPath(joinPath(spath, "events"), std::to_string(e)), error))
                        return false;
                    seg.events.push_back(std::move(ev));
                }
            }
            if (!s.contains("exit")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing exit", "", joinPath(spath, "exit"));
                return false;
            }
            if (!parseTrigger(s.at("exit"), seg.exit, joinPath(spath, "exit"), error))
                return false;
            readBool(s, "bossRoom", seg.bossRoom, spath, error, false);
            if (s.contains("cameraBounds")) {
                CameraBounds bounds;
                if (!parseBounds(s.at("cameraBounds"), bounds, joinPath(spath, "cameraBounds"), error))
                    return false;
                seg.cameraBounds = bounds;
            }
            out.push_back(std::move(seg));
        }
        return true;
    }

    bool parseArchetypes(const json& j, std::vector<LevelArchetype>& out, const std::string& path,
                         LevelLoadError& error)
    {
        if (!j.is_array()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected archetypes array", "", path);
            return false;
        }
        out.clear();
        out.reserve(j.size());
        for (std::size_t i = 0; i < j.size(); ++i) {
            const json& a     = j[i];
            std::string apath = joinPath(path, std::to_string(i));
            if (!a.is_object()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid archetype object", "", apath);
                return false;
            }
            std::int32_t typeId = 0;
            std::string spriteId;
            std::string animId;
            std::int32_t layer = 0;
            if (!readInt(a, "typeId", typeId, apath, error, true))
                return false;
            if (!readString(a, "spriteId", spriteId, apath, error, true))
                return false;
            if (!readString(a, "animId", animId, apath, error, true))
                return false;
            if (!readInt(a, "layer", layer, apath, error, true))
                return false;
            LevelArchetype archetype{};
            archetype.typeId   = static_cast<std::uint16_t>(typeId);
            archetype.spriteId = spriteId;
            archetype.animId   = animId;
            archetype.layer    = static_cast<std::uint8_t>(layer);
            out.push_back(std::move(archetype));
        }
        return true;
    }

    bool parseMeta(const json& j, LevelMeta& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected meta object", "", path);
            return false;
        }
        if (!readString(j, "backgroundId", out.backgroundId, path, error, true))
            return false;
        if (!readString(j, "musicId", out.musicId, path, error, true))
            return false;
        readString(j, "name", out.name, path, error, false);
        readString(j, "author", out.author, path, error, false);
        readString(j, "difficulty", out.difficulty, path, error, false);
        return true;
    }

    bool validateUnique(const std::vector<PatternDefinition>& patterns, LevelLoadError& error)
    {
        std::unordered_set<std::string> ids;
        for (const auto& p : patterns) {
            if (!ids.insert(p.id).second) {
                setError(error, LevelLoadErrorCode::SemanticError, "Duplicate pattern id: " + p.id, "", "");
                return false;
            }
        }
        return true;
    }

    bool validateSegments(const std::vector<LevelSegment>& segments, LevelLoadError& error)
    {
        std::unordered_set<std::string> ids;
        for (const auto& s : segments) {
            if (!ids.insert(s.id).second) {
                setError(error, LevelLoadErrorCode::SemanticError, "Duplicate segment id: " + s.id, "", "");
                return false;
            }
        }
        return true;
    }

    void collectSpawnIds(const LevelEvent& ev, std::unordered_set<std::string>& spawnIds)
    {
        if (ev.type == EventType::SpawnWave) {
            if (!ev.id.empty())
                spawnIds.insert(ev.id);
        } else if (ev.type == EventType::SpawnObstacle) {
            if (ev.obstacle && !ev.obstacle->spawnId.empty())
                spawnIds.insert(ev.obstacle->spawnId);
            else if (!ev.id.empty())
                spawnIds.insert(ev.id);
        } else if (ev.type == EventType::SpawnBoss) {
            if (ev.boss && !ev.boss->spawnId.empty())
                spawnIds.insert(ev.boss->spawnId);
            else if (!ev.id.empty())
                spawnIds.insert(ev.id);
        }
    }

    void collectCheckpointIds(const LevelEvent& ev, std::unordered_set<std::string>& checkpointIds)
    {
        if (ev.type == EventType::Checkpoint && ev.checkpoint) {
            if (!ev.checkpoint->checkpointId.empty())
                checkpointIds.insert(ev.checkpoint->checkpointId);
        }
    }

    bool validateScrollCurve(const ScrollSettings& scroll, LevelLoadError& error)
    {
        if (scroll.mode != ScrollMode::Curve)
            return true;
        if (scroll.curve.empty()) {
            setError(error, LevelLoadErrorCode::SemanticError, "Empty scroll curve", "", "");
            return false;
        }
        if (scroll.curve.front().time != 0.0F) {
            setError(error, LevelLoadErrorCode::SemanticError, "Scroll curve must start at time 0", "", "");
            return false;
        }
        for (std::size_t i = 1; i < scroll.curve.size(); ++i) {
            if (scroll.curve[i].time < scroll.curve[i - 1].time) {
                setError(error, LevelLoadErrorCode::SemanticError, "Scroll curve must be sorted by time", "", "");
                return false;
            }
        }
        return true;
    }

    bool validateEvent(const LevelEvent& ev, const LevelData& data, const std::unordered_set<std::string>& spawnIds,
                       const std::unordered_set<std::string>& checkpointIds, LevelLoadError& error)
    {
        if (ev.repeat) {
            if (!std::isfinite(ev.repeat->interval) || ev.repeat->interval <= 0.0F) {
                setError(error, LevelLoadErrorCode::SemanticError, "Repeat interval must be > 0", "", "");
                return false;
            }
            if (!ev.repeat->count.has_value() && !ev.repeat->until.has_value()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Repeat requires count or until", "", "");
                return false;
            }
        }

        if (ev.type == EventType::SpawnWave && ev.wave) {
            const auto& wave  = *ev.wave;
            bool patternFound = false;
            for (const auto& p : data.patterns) {
                if (p.id == wave.patternId) {
                    patternFound = true;
                    break;
                }
            }
            if (!patternFound) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown patternId: " + wave.patternId, "", "");
                return false;
            }
            if (data.templates.enemies.find(wave.enemy) == data.templates.enemies.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown enemy template: " + wave.enemy, "", "");
                return false;
            }
        }

        if (ev.type == EventType::SpawnObstacle && ev.obstacle) {
            const auto& ob = *ev.obstacle;
            auto it        = data.templates.obstacles.find(ob.obstacle);
            if (it == data.templates.obstacles.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown obstacle template: " + ob.obstacle, "", "");
                return false;
            }
            ObstacleAnchor anchor = it->second.anchor;
            if (ob.anchor.has_value())
                anchor = *ob.anchor;
            if (anchor == ObstacleAnchor::Absolute && !ob.y.has_value()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Absolute obstacle requires y", "", "");
                return false;
            }
        }

        if (ev.type == EventType::SpawnBoss && ev.boss) {
            const auto& boss = *ev.boss;
            if (data.bosses.find(boss.bossId) == data.bosses.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown bossId: " + boss.bossId, "", "");
                return false;
            }
        }

        if (ev.type == EventType::Checkpoint && ev.checkpoint) {
            if (checkpointIds.find(ev.checkpoint->checkpointId) == checkpointIds.end()) {
                setError(error, LevelLoadErrorCode::SemanticError,
                         "Checkpoint id not registered: " + ev.checkpoint->checkpointId, "", "");
                return false;
            }
        }

        if (ev.type == EventType::GateOpen || ev.type == EventType::GateClose) {
            if (ev.gateId && spawnIds.find(*ev.gateId) == spawnIds.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "GateId does not match any spawnId: " + *ev.gateId,
                         "", "");
                return false;
            }
        }

        return true;
    }

    bool validateTrigger(const Trigger& trigger, const std::unordered_set<std::string>& spawnIds,
                         const std::unordered_set<std::string>& checkpointIds,
                         const std::unordered_map<std::string, BossDefinition>& bosses, LevelLoadError& error)
    {
        if (trigger.type == TriggerType::SpawnDead) {
            if (spawnIds.find(trigger.spawnId) == spawnIds.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown spawnId in trigger: " + trigger.spawnId, "",
                         "");
                return false;
            }
        }
        if (trigger.type == TriggerType::BossDead || trigger.type == TriggerType::HpBelow) {
            if (bosses.find(trigger.bossId) == bosses.end()) {
                setError(error, LevelLoadErrorCode::SemanticError, "Unknown bossId in trigger: " + trigger.bossId, "",
                         "");
                return false;
            }
        }
        if (trigger.type == TriggerType::CheckpointReached) {
            if (checkpointIds.find(trigger.checkpointId) == checkpointIds.end()) {
                setError(error, LevelLoadErrorCode::SemanticError,
                         "Unknown checkpointId in trigger: " + trigger.checkpointId, "", "");
                return false;
            }
        }
        if (trigger.type == TriggerType::AllOf || trigger.type == TriggerType::AnyOf) {
            for (const auto& child : trigger.triggers) {
                if (!validateTrigger(child, spawnIds, checkpointIds, bosses, error))
                    return false;
            }
        }
        return true;
    }

    bool validateScales(const LevelData& data, LevelLoadError& error)
    {
        auto checkScale = [&](const Vec2f& scale, const std::string& label) -> bool {
            if (!finiteVec(scale) || scale.x <= 0.0F || scale.y <= 0.0F) {
                setError(error, LevelLoadErrorCode::SemanticError, "Invalid scale: " + label, "", "");
                return false;
            }
            return true;
        };
        for (const auto& [id, enemy] : data.templates.enemies) {
            if (!checkScale(enemy.scale, "enemy:" + id))
                return false;
        }
        for (const auto& [id, obstacle] : data.templates.obstacles) {
            if (!checkScale(obstacle.scale, "obstacle:" + id))
                return false;
        }
        for (const auto& [id, boss] : data.bosses) {
            if (!checkScale(boss.scale, "boss:" + id))
                return false;
        }
        for (const auto& seg : data.segments) {
            for (const auto& ev : seg.events) {
                if (ev.wave && ev.wave->scale) {
                    if (!checkScale(*ev.wave->scale, "wave"))
                        return false;
                }
                if (ev.obstacle && ev.obstacle->scale) {
                    if (!checkScale(*ev.obstacle->scale, "spawn_obstacle"))
                        return false;
                }
            }
        }
        return true;
    }

    struct RequiredArchetype
    {
        std::uint16_t typeId;
        const char* label;
    };

    const RequiredArchetype kRequiredArchetypes[] = {
        {1, "player1"},
        {12, "player2"},
        {13, "player3"},
        {14, "player4"},
        {3, "bullet_basic"},
        {4, "bullet_charge_lvl1"},
        {5, "bullet_charge_lvl2"},
        {6, "bullet_charge_lvl3"},
        {7, "bullet_charge_lvl4"},
        {8, "bullet_charge_lvl5"},
        {15, "enemy_bullet_basic"},
        {16, "player_death"},
    };

    bool validateArchetypes(const LevelData& data, LevelLoadError& error)
    {
        std::unordered_set<std::uint16_t> typeIds;
        typeIds.reserve(data.archetypes.size());
        for (const auto& archetype : data.archetypes) {
            if (!typeIds.insert(archetype.typeId).second) {
                setError(error, LevelLoadErrorCode::SemanticError,
                         "Duplicate archetype typeId: " + std::to_string(archetype.typeId), "", "");
                return false;
            }
        }

        for (const auto& req : kRequiredArchetypes) {
            if (typeIds.find(req.typeId) == typeIds.end()) {
                setError(error, LevelLoadErrorCode::SemanticError,
                         "Missing required archetype typeId: " + std::to_string(req.typeId) + " (" + req.label + ")",
                         "", "");
                return false;
            }
        }

        auto ensureType = [&](std::uint16_t typeId, const std::string& label) -> bool {
            if (typeIds.find(typeId) == typeIds.end()) {
                setError(error, LevelLoadErrorCode::SemanticError,
                         "Missing archetype for " + label + " typeId: " + std::to_string(typeId), "", "");
                return false;
            }
            return true;
        };

        for (const auto& [id, enemy] : data.templates.enemies) {
            if (!ensureType(enemy.typeId, "enemy template " + id))
                return false;
        }
        for (const auto& [id, obstacle] : data.templates.obstacles) {
            if (!ensureType(obstacle.typeId, "obstacle template " + id))
                return false;
        }
        for (const auto& [id, boss] : data.bosses) {
            if (!ensureType(boss.typeId, "boss " + id))
                return false;
        }

        return true;
    }

    bool validateLevel(const LevelData& data, LevelLoadError& error)
    {
        if (!validateUnique(data.patterns, error))
            return false;
        if (!validateSegments(data.segments, error))
            return false;
        if (!validateArchetypes(data, error))
            return false;

        std::unordered_set<std::string> checkpointIds;
        std::unordered_set<std::string> spawnIds;
        for (const auto& seg : data.segments) {
            if (!validateScrollCurve(seg.scroll, error))
                return false;
            for (const auto& ev : seg.events) {
                collectCheckpointIds(ev, checkpointIds);
                collectSpawnIds(ev, spawnIds);
            }
        }
        for (const auto& [_, boss] : data.bosses) {
            for (const auto& phase : boss.phases) {
                for (const auto& ev : phase.events) {
                    collectCheckpointIds(ev, checkpointIds);
                    collectSpawnIds(ev, spawnIds);
                }
            }
            for (const auto& ev : boss.onDeath) {
                collectCheckpointIds(ev, checkpointIds);
                collectSpawnIds(ev, spawnIds);
            }
        }

        for (const auto& seg : data.segments) {
            if (!validateTrigger(seg.exit, spawnIds, checkpointIds, data.bosses, error))
                return false;
            for (const auto& ev : seg.events) {
                if (!validateTrigger(ev.trigger, spawnIds, checkpointIds, data.bosses, error))
                    return false;
                if (!validateEvent(ev, data, spawnIds, checkpointIds, error))
                    return false;
            }
        }

        for (const auto& [_, boss] : data.bosses) {
            for (const auto& phase : boss.phases) {
                if (!validateTrigger(phase.trigger, spawnIds, checkpointIds, data.bosses, error))
                    return false;
                for (const auto& ev : phase.events) {
                    if (!validateTrigger(ev.trigger, spawnIds, checkpointIds, data.bosses, error))
                        return false;
                    if (!validateEvent(ev, data, spawnIds, checkpointIds, error))
                        return false;
                }
            }
            for (const auto& ev : boss.onDeath) {
                if (!validateTrigger(ev.trigger, spawnIds, checkpointIds, data.bosses, error))
                    return false;
                if (!validateEvent(ev, data, spawnIds, checkpointIds, error))
                    return false;
            }
        }

        if (!validateScales(data, error))
            return false;

        return true;
    }

    bool parseLevel(const json& root, LevelData& out, LevelLoadError& error, const std::string& path)
    {
        if (!root.is_object()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected object", path, "");
            return false;
        }

        std::int32_t schemaVersion = 0;
        if (!readInt(root, "schemaVersion", schemaVersion, "", error, true))
            return false;
        if (schemaVersion != 1) {
            setError(error, LevelLoadErrorCode::SchemaError, "Unsupported schemaVersion", path, "/schemaVersion");
            return false;
        }

        std::int32_t levelId = 0;
        if (!readInt(root, "levelId", levelId, "", error, true))
            return false;

        const json* meta       = nullptr;
        const json* archetypes = nullptr;
        const json* patterns   = nullptr;
        const json* templates  = nullptr;
        const json* segments   = nullptr;

        if (!requireObject(root, "meta", meta, "", error))
            return false;
        if (!requireArray(root, "archetypes", archetypes, "", error))
            return false;
        if (!requireArray(root, "patterns", patterns, "", error))
            return false;
        if (!requireObject(root, "templates", templates, "", error))
            return false;
        if (!requireArray(root, "segments", segments, "", error))
            return false;

        LevelMeta metaData;
        if (!parseMeta(*meta, metaData, "/meta", error))
            return false;

        std::vector<LevelArchetype> archetypeData;
        if (!parseArchetypes(*archetypes, archetypeData, "/archetypes", error))
            return false;

        std::vector<PatternDefinition> patternData;
        if (!parsePatterns(*patterns, patternData, "/patterns", error))
            return false;

        LevelTemplates templateData;
        if (!parseTemplates(*templates, templateData, "/templates", error))
            return false;

        std::unordered_map<std::string, BossDefinition> bosses;
        if (root.contains("bosses")) {
            if (!parseBosses(root.at("bosses"), templateData, bosses, "/bosses", error))
                return false;
        }

        std::vector<LevelSegment> segmentData;
        if (!parseSegments(*segments, segmentData, "/segments", error))
            return false;

        out.schemaVersion = schemaVersion;
        out.levelId       = levelId;
        out.meta          = metaData;
        out.archetypes    = std::move(archetypeData);
        out.patterns      = std::move(patternData);
        out.templates     = std::move(templateData);
        out.bosses        = std::move(bosses);
        out.segments      = std::move(segmentData);

        return validateLevel(out, error);
    }

    bool parseRegistry(const json& root, LevelRegistry& out, LevelLoadError& error)
    {
        if (!root.is_object()) {
            setError(error, LevelLoadErrorCode::RegistryError, "Registry is not an object", "", "");
            return false;
        }
        std::int32_t schemaVersion = 0;
        if (!readInt(root, "schemaVersion", schemaVersion, "", error, true))
            return false;
        if (schemaVersion != 1) {
            setError(error, LevelLoadErrorCode::RegistryError, "Unsupported registry schemaVersion", "",
                     "/schemaVersion");
            return false;
        }
        const json* levels = nullptr;
        if (!requireArray(root, "levels", levels, "", error))
            return false;
        out.schemaVersion = schemaVersion;
        out.levels.clear();
        out.levels.reserve(levels->size());
        std::unordered_set<std::int32_t> ids;
        for (std::size_t i = 0; i < levels->size(); ++i) {
            const json& entry = (*levels)[i];
            std::string epath = joinPath("/levels", std::to_string(i));
            if (!entry.is_object()) {
                setError(error, LevelLoadErrorCode::RegistryError, "Invalid registry entry", "", epath);
                return false;
            }
            std::int32_t id = 0;
            std::string path;
            std::string name;
            if (!readInt(entry, "id", id, epath, error, true))
                return false;
            if (!readString(entry, "path", path, epath, error, true))
                return false;
            readString(entry, "name", name, epath, error, false);
            if (!ids.insert(id).second) {
                setError(error, LevelLoadErrorCode::RegistryError, "Duplicate level id in registry", "", epath);
                return false;
            }
            out.levels.push_back(LevelRegistryEntry{id, path, name});
        }
        return true;
    }
} // namespace

std::string LevelLoader::levelsRoot()
{
    return "server/assets/levels";
}

bool LevelLoader::loadRegistry(LevelRegistry& out, LevelLoadError& error)
{
    std::filesystem::path root         = levelsRoot();
    std::filesystem::path registryPath = root / "registry.json";
    if (!std::filesystem::exists(registryPath)) {
        setError(error, LevelLoadErrorCode::RegistryError, "Registry file not found", registryPath.string(), "");
        return false;
    }
    error.path = registryPath.string();
    std::string text;
    if (!readFile(registryPath.string(), text, error))
        return false;
    json doc;
    if (!parseJson(text, doc, error, registryPath.string()))
        return false;
    return parseRegistry(doc, out, error);
}

bool LevelLoader::loadFromPath(const std::string& path, LevelData& out, LevelLoadError& error)
{
    error.path = path;
    std::string text;
    if (!readFile(path, text, error))
        return false;
    json doc;
    if (!parseJson(text, doc, error, path))
        return false;
    if (!parseLevel(doc, out, error, path))
        return false;
    return true;
}

bool LevelLoader::load(std::int32_t levelId, LevelData& out, LevelLoadError& error)
{
    std::filesystem::path root         = levelsRoot();
    std::filesystem::path registryPath = root / "registry.json";
    if (std::filesystem::exists(registryPath)) {
        LevelRegistry registry;
        if (!loadRegistry(registry, error))
            return false;
        for (const auto& entry : registry.levels) {
            if (entry.id == levelId) {
                std::filesystem::path fullPath = root / entry.path;
                return loadFromPath(fullPath.string(), out, error);
            }
        }
        setError(error, LevelLoadErrorCode::RegistryError, "Level id not found in registry", registryPath.string(), "");
        return false;
    }

    std::filesystem::path direct = root / ("level_" + std::to_string(levelId) + ".json");
    if (std::filesystem::exists(direct))
        return loadFromPath(direct.string(), out, error);
    std::ostringstream padded;
    padded << "level_" << std::setw(2) << std::setfill('0') << levelId << ".json";
    std::filesystem::path paddedPath = root / padded.str();
    if (std::filesystem::exists(paddedPath))
        return loadFromPath(paddedPath.string(), out, error);

    setError(error, LevelLoadErrorCode::FileNotFound, "Level file not found", direct.string(), "");
    return false;
}
