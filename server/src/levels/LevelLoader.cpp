#include "levels/LevelLoader.hpp"

#include "components/HitboxComponent.hpp"
#include "components/MovementComponent.hpp"
#include "components/ScoreComponent.hpp"
#include "levels/LevelData.hpp"

#include "json/Json.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    using Json = rtype::Json;

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

    bool parseJson(const std::string& text, Json& out, LevelLoadError& error, const std::string& path)
    {
        try {
            out = Json::parse(text);
            return true;
        } catch (const rtype::JsonParseError& e) {
            setError(error, LevelLoadErrorCode::JsonParseError, e.what(), path, "");
            return false;
        }
    }

    bool requireObject(const Json& obj, const std::string& key, Json& out, const std::string& path,
                       LevelLoadError& error)
    {
        if (!obj.contains(key)) {
            setError(error, LevelLoadErrorCode::SchemaError, "Missing object: " + key, "", joinPath(path, key));
            return false;
        }
        Json value = obj[key];
        if (!value.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected object: " + key, "", joinPath(path, key));
            return false;
        }
        out = value;
        return true;
    }

    bool requireArray(const Json& obj, const std::string& key, Json& out, const std::string& path,
                      LevelLoadError& error)
    {
        if (!obj.contains(key)) {
            setError(error, LevelLoadErrorCode::SchemaError, "Missing array: " + key, "", joinPath(path, key));
            return false;
        }
        Json value = obj[key];
        if (!value.isArray()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected array: " + key, "", joinPath(path, key));
            return false;
        }
        out = value;
        return true;
    }

    bool readString(const Json& obj, const std::string& key, std::string& out, const std::string& path,
                    LevelLoadError& error, bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing string: " + key, "", joinPath(path, key));
            return false;
        }
        Json value = obj[key];
        if (!value.isString()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected string: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<std::string>();
        return true;
    }

    bool readBool(const Json& obj, const std::string& key, bool& out, const std::string& path, LevelLoadError& error,
                  bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing bool: " + key, "", joinPath(path, key));
            return false;
        }
        Json value = obj[key];
        if (!value.isBoolean()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected bool: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<bool>();
        return true;
    }

    bool readNumber(const Json& obj, const std::string& key, double& out, const std::string& path,
                    LevelLoadError& error, bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing number: " + key, "", joinPath(path, key));
            return false;
        }
        Json value = obj[key];
        if (!value.isNumber()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected number: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<double>();
        return true;
    }

    bool readInt(const Json& obj, const std::string& key, std::int32_t& out, const std::string& path,
                 LevelLoadError& error, bool required)
    {
        if (!obj.contains(key)) {
            if (!required)
                return true;
            setError(error, LevelLoadErrorCode::SchemaError, "Missing integer: " + key, "", joinPath(path, key));
            return false;
        }
        Json value = obj[key];
        if (!value.isNumberInteger() && !value.isNumberUnsigned()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected integer: " + key, "", joinPath(path, key));
            return false;
        }
        out = value.get<std::int32_t>();
        return true;
    }

    bool parseVec2(const Json& j, Vec2f& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isArray() || j.size() != 2) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected [x,y] array", "", path);
            return false;
        }
        Json v0 = j[0];
        Json v1 = j[1];
        if (!v0.isNumber() || !v1.isNumber()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected numeric vector2", "", path);
            return false;
        }
        out.x = v0.get<float>();
        out.y = v1.get<float>();
        return true;
    }

    bool parseVec2ObjectOrArray(const Json& j, Vec2f& out, const std::string& path, LevelLoadError& error)
    {
        if (j.isArray()) {
            return parseVec2(j, out, path, error);
        }
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected {x,y} object", "", path);
            return false;
        }
        double x = 0.0;
        double y = 0.0;
        if (!readNumber(j, "x", x, path, error, true))
            return false;
        if (!readNumber(j, "y", y, path, error, true))
            return false;
        out.x = static_cast<float>(x);
        out.y = static_cast<float>(y);
        return true;
    }

    bool finiteVec(const Vec2f& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y);
    }

    bool parseHitbox(const Json& j, HitboxComponent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
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

    bool parseCollider(const Json& j, ColliderComponent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
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
            if (!j.contains("points")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing polygon points", "",
                         joinPath(path, "points"));
                return false;
            }
            Json pts = j["points"];
            if (!pts.isArray()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Expected points array", "", joinPath(path, "points"));
                return false;
            }
            if (pts.size() < 3) {
                setError(error, LevelLoadErrorCode::SchemaError, "Polygon needs at least 3 points", "", path);
                return false;
            }
            std::vector<std::array<float, 2>> points;
            points.reserve(pts.size());
            for (std::size_t i = 0; i < pts.size(); ++i) {
                Json p = pts[i];
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

    bool parseShooting(const Json& j, EnemyShootingComponent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected shooting object", "", path);
            return false;
        }

        double interval = 1.5;
        double speed    = 300.0;
        double damage   = 5.0;
        double lifetime = 3.0;

        readNumber(j, "interval", interval, path, error, false);
        readNumber(j, "speed", speed, path, error, false);
        readNumber(j, "damage", damage, path, error, false);
        readNumber(j, "lifetime", lifetime, path, error, false);

        out = EnemyShootingComponent::create(static_cast<float>(interval), static_cast<float>(speed),
                                             static_cast<std::int32_t>(damage), static_cast<float>(lifetime));
        return true;
    }

    bool parseScroll(const Json& j, ScrollSettings& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected scroll object", "", path);
            return false;
        }

        std::string modeStr;
        readString(j, "mode", modeStr, path, error, false);

        ScrollMode mode = ScrollMode::Constant;
        if (modeStr == "stopped")
            mode = ScrollMode::Stopped;
        else if (modeStr == "curve")
            mode = ScrollMode::Curve;
        else
            mode = ScrollMode::Constant;

        double speedX = 0.0;
        readNumber(j, "speedX", speedX, path, error, false);
        if (j.contains("speed")) {
            readNumber(j, "speed", speedX, path, error, false);
        }

        std::vector<ScrollKeyframe> curve;
        if (mode == ScrollMode::Curve && j.contains("curve")) {
            Json curveArr;
            if (requireArray(j, "curve", curveArr, path, error)) {
                for (size_t i = 0; i < curveArr.size(); ++i) {
                    Json p      = curveArr[i];
                    double time = 0.0;
                    double sx   = 0.0;
                    readNumber(p, "time", time, path, error, true);
                    readNumber(p, "speedX", sx, path, error, true);
                    curve.push_back({static_cast<float>(time), static_cast<float>(sx)});
                }
            } else {
                return false;
            }
        }

        out.mode   = mode;
        out.speedX = static_cast<float>(speedX);
        out.curve  = std::move(curve);
        return true;
    }

    bool parseBounds(const Json& j, CameraBounds& out, const std::string& path, LevelLoadError& error);
    bool parseTrigger(const Json& j, Trigger& out, const std::string& path, LevelLoadError& error);

    bool parseRepeat(const Json& j, RepeatSpec& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected repeat object", "", path);
            return false;
        }
        if (j.contains("count")) {
            std::int32_t c = 0;
            if (readInt(j, "count", c, path, error, true))
                out.count = c;
            else
                return false;
        }

        double iv = 0.0;
        if (readNumber(j, "interval", iv, path, error, true))
            out.interval = static_cast<float>(iv);
        else
            return false;

        if (j.contains("until")) {
            Json u;
            if (requireObject(j, "until", u, path, error)) {
                Trigger t;
                if (parseTrigger(u, t, joinPath(path, "until"), error))
                    out.until = t;
                else
                    return false;
            } else
                return false;
        }
        return true;
    }

    bool parseTrigger(const Json& j, Trigger& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected trigger object", "", path);
            return false;
        }

        std::string type;
        if (!readString(j, "type", type, path, error, true))
            return false;

        if (type == "time") {
            out.type   = TriggerType::Time;
            double val = 0.0;
            readNumber(j, "value", val, path, error, false);
            if (j.contains("time"))
                readNumber(j, "time", val, path, error, false);
            out.time = static_cast<float>(val);
            return true;
        }
        if (type == "distance") {
            out.type = TriggerType::Distance;
            double d = 0.0;
            readNumber(j, "distance", d, path, error, true);
            out.distance = static_cast<float>(d);
            return true;
        }
        if (type == "spawn_dead") {
            out.type = TriggerType::SpawnDead;
            readString(j, "spawnId", out.spawnId, path, error, true);
            return true;
        }
        if (type == "boss_dead") {
            out.type = TriggerType::BossDead;
            readString(j, "bossId", out.bossId, path, error, true);
            return true;
        }
        if (type == "checkpoint_reached") {
            out.type = TriggerType::CheckpointReached;
            readString(j, "checkpointId", out.checkpointId, path, error, true);
            return true;
        }
        if (type == "hp_below") {
            out.type = TriggerType::HpBelow;
            readString(j, "bossId", out.bossId, path, error, true);
            std::int32_t val = 0;
            readInt(j, "value", val, path, error, true);
            out.value = val;
            return true;
        }
        if (type == "enemy_count_at_most") {
            out.type       = TriggerType::EnemyCountAtMost;
            std::int32_t c = 0;
            readInt(j, "count", c, path, error, true);
            out.count = c;
            return true;
        }
        if (type == "player_in_zone") {
            out.type = TriggerType::PlayerInZone;
            if (!j.contains("bounds")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing bounds", "", joinPath(path, "bounds"));
                return false;
            }
            if (!parseBounds(j["bounds"], out.zone.emplace(), joinPath(path, "bounds"), error))
                return false;
            if (j.contains("requireAll")) {
                bool requireAll = false;
                if (!readBool(j, "requireAll", requireAll, path, error, true))
                    return false;
                out.requireAllPlayers = requireAll;
            }
            return true;
        }
        if (type == "and" || type == "all_of") {
            out.type = TriggerType::AllOf;
            Json triggers;
            if (!requireArray(j, "triggers", triggers, path, error))
                return false;
            for (size_t i = 0; i < triggers.size(); ++i) {
                Trigger sub;
                if (!parseTrigger(triggers[i], sub, joinPath(path, "triggers"), error))
                    return false;
                out.triggers.push_back(sub);
            }
            return true;
        }
        if (type == "or" || type == "any_of") {
            out.type = TriggerType::AnyOf;
            Json triggers;
            if (!requireArray(j, "triggers", triggers, path, error))
                return false;
            for (size_t i = 0; i < triggers.size(); ++i) {
                Trigger sub;
                if (!parseTrigger(triggers[i], sub, joinPath(path, "triggers"), error))
                    return false;
                out.triggers.push_back(sub);
            }
            return true;
        }

        setError(error, LevelLoadErrorCode::SchemaError, "Unknown trigger type: " + type, "", path);
        return false;
    }

    bool parseWave(const Json& j, WaveDefinition& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
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
            if (!parseVec2(j["scale"], scale, joinPath(path, "scale"), error))
                return false;
            out.scale = scale;
        }
        if (j.contains("shootingEnabled")) {
            bool shootingEnabled = true;
            if (!readBool(j, "shootingEnabled", shootingEnabled, path, error, true))
                return false;
            out.shootingEnabled = shootingEnabled;
        }

        std::int32_t count = 1;
        readInt(j, "count", count, path, error, false);
        out.count = count;

        double val = 0;
        if (j.contains("spawnX")) {
            readNumber(j, "spawnX", val, path, error, false);
            out.spawnX = static_cast<float>(val);
        }
        if (j.contains("startY")) {
            readNumber(j, "startY", val, path, error, false);
            out.startY = static_cast<float>(val);
        }
        if (j.contains("deltaY")) {
            readNumber(j, "deltaY", val, path, error, false);
            out.deltaY = static_cast<float>(val);
        }
        if (j.contains("stepY")) {
            readNumber(j, "stepY", val, path, error, false);
            out.stepY = static_cast<float>(val);
        }

        if (j.contains("spacing")) {
            readNumber(j, "spacing", val, path, error, false);
            out.spacing = static_cast<float>(val);
        }
        if (j.contains("stepTime")) {
            readNumber(j, "stepTime", val, path, error, false);
            out.stepTime = static_cast<float>(val);
        }

        if (type == "line") {
            out.type = WaveType::Line;
        } else if (type == "stagger") {
            out.type = WaveType::Stagger;
            if (j.contains("interval")) {
                readNumber(j, "interval", val, path, error, false);
                out.stepTime = static_cast<float>(val);
            }
        } else if (type == "triangle") {
            out.type            = WaveType::Triangle;
            std::int32_t layers = 1;
            readInt(j, "layers", layers, path, error, false);
            out.layers = layers;
            if (j.contains("rowHeight")) {
                readNumber(j, "rowHeight", val, path, error, false);
                out.rowHeight = static_cast<float>(val);
            }
            if (j.contains("horizontalStep")) {
                readNumber(j, "horizontalStep", val, path, error, false);
                out.horizontalStep = static_cast<float>(val);
            }
            if (j.contains("apexY")) {
                readNumber(j, "apexY", val, path, error, false);
                out.apexY = static_cast<float>(val);
            }
        } else if (type == "serpent") {
            out.type = WaveType::Serpent;
            if (j.contains("amplitudeX")) {
                readNumber(j, "amplitudeX", val, path, error, false);
                out.amplitudeX = static_cast<float>(val);
            }
        } else if (type == "cross") {
            out.type = WaveType::Cross;
            if (j.contains("centerX")) {
                readNumber(j, "centerX", val, path, error, false);
                out.centerX = static_cast<float>(val);
            }
            if (j.contains("centerY")) {
                readNumber(j, "centerY", val, path, error, false);
                out.centerY = static_cast<float>(val);
            }
            if (j.contains("step")) {
                readNumber(j, "step", val, path, error, false);
                out.step = static_cast<float>(val);
            }
            std::int32_t al = 0;
            readInt(j, "armLength", al, path, error, false);
            out.armLength = al;
        } else {
            out.type = WaveType::Line;
        }

        return true;
    }

    bool parseBounds(const Json& j, CameraBounds& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected bounds object", "", path);
            return false;
        }
        double x = 0, y = 0, width = 0, height = 0;
        if (j.contains("minX")) {
            readNumber(j, "minX", x, path, error, true);
            out.minX = static_cast<float>(x);
            readNumber(j, "maxX", width, path, error, true);
            out.maxX = static_cast<float>(width);
            readNumber(j, "minY", y, path, error, true);
            out.minY = static_cast<float>(y);
            readNumber(j, "maxY", height, path, error, true);
            out.maxY = static_cast<float>(height);
        } else {
            if (!readNumber(j, "x", x, path, error, true))
                return false;
            if (!readNumber(j, "y", y, path, error, true))
                return false;
            if (!readNumber(j, "width", width, path, error, true))
                return false;
            if (!readNumber(j, "height", height, path, error, true))
                return false;
            out.minX = static_cast<float>(x);
            out.minY = static_cast<float>(y);
            out.maxX = static_cast<float>(x + width);
            out.maxY = static_cast<float>(y + height);
        }
        return true;
    }

    bool parseEvent(const Json& j, LevelEvent& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected event object", "", path);
            return false;
        }
        std::string type;
        if (!readString(j, "type", type, path, error, true))
            return false;

        if (type == "spawn_wave")
            out.type = EventType::SpawnWave;
        else if (type == "spawn_obstacle")
            out.type = EventType::SpawnObstacle;
        else if (type == "spawn_boss")
            out.type = EventType::SpawnBoss;
        else if (type == "set_scroll")
            out.type = EventType::SetScroll;
        else if (type == "set_background")
            out.type = EventType::SetBackground;
        else if (type == "set_music")
            out.type = EventType::SetMusic;
        else if (type == "set_camera_bounds")
            out.type = EventType::SetCameraBounds;
        else if (type == "set_player_bounds")
            out.type = EventType::SetPlayerBounds;
        else if (type == "clear_player_bounds")
            out.type = EventType::ClearPlayerBounds;
        else if (type == "gate_open")
            out.type = EventType::GateOpen;
        else if (type == "gate_close")
            out.type = EventType::GateClose;
        else if (type == "checkpoint")
            out.type = EventType::Checkpoint;
        else {
            setError(error, LevelLoadErrorCode::SchemaError, "Unknown event type: " + type, "", path);
            return false;
        }

        out.id = "";
        if (j.contains("id"))
            readString(j, "id", out.id, path, error, false);

        if (j.contains("trigger")) {
            Json t;
            if (requireObject(j, "trigger", t, path, error)) {
                if (!parseTrigger(t, out.trigger, joinPath(path, "trigger"), error))
                    return false;
            } else
                return false;
        }

        if (j.contains("repeat")) {
            Json r;
            if (requireObject(j, "repeat", r, path, error)) {
                RepeatSpec rs;
                if (parseRepeat(r, rs, joinPath(path, "repeat"), error))
                    out.repeat = rs;
                else
                    return false;
            }
        }

        if (out.type == EventType::SpawnWave && j.contains("wave")) {
            Json w;
            if (requireObject(j, "wave", w, path, error)) {
                if (!parseWave(w, out.wave.emplace(), joinPath(path, "wave"), error))
                    return false;
            }
        }
        if (out.type == EventType::SetScroll && j.contains("scroll")) {
            Json s;
            if (requireObject(j, "scroll", s, path, error)) {
                if (!parseScroll(s, out.scroll.emplace(), joinPath(path, "scroll"), error))
                    return false;
            }
        }
        if (out.type == EventType::SetCameraBounds && j.contains("bounds")) {
            Json b;
            if (requireObject(j, "bounds", b, path, error)) {
                if (!parseBounds(b, out.cameraBounds.emplace(), joinPath(path, "bounds"), error))
                    return false;
            }
        }
        if (out.type == EventType::SetPlayerBounds && j.contains("bounds")) {
            Json b;
            if (requireObject(j, "bounds", b, path, error)) {
                if (!parseBounds(b, out.playerBounds.emplace(), joinPath(path, "bounds"), error))
                    return false;
            }
        }
        if (out.type == EventType::SetBackground && j.contains("backgroundId")) {
            readString(j, "backgroundId", out.backgroundId.emplace(), path, error, true);
        }
        if (out.type == EventType::SetMusic && j.contains("musicId")) {
            readString(j, "musicId", out.musicId.emplace(), path, error, true);
        }
        if ((out.type == EventType::GateOpen || out.type == EventType::GateClose) && j.contains("gateId")) {
            readString(j, "gateId", out.gateId.emplace(), path, error, true);
        }

        if (out.type == EventType::Checkpoint) {
            CheckpointDefinition cp;
            if (!readString(j, "checkpointId", cp.checkpointId, path, error, true))
                return false;
            if (j.contains("respawn")) {
                if (!parseVec2ObjectOrArray(j["respawn"], cp.respawn, joinPath(path, "respawn"), error))
                    return false;
            }
            out.checkpoint = cp;
        }

        if (out.type == EventType::SpawnObstacle) {
            SpawnObstacleSettings obs;
            if (!readString(j, "obstacle", obs.obstacle, path, error, true))
                return false;

            double val = 0.0;
            if (!readNumber(j, "x", val, path, error, true))
                return false;
            obs.x = static_cast<float>(val);

            if (j.contains("y")) {
                readNumber(j, "y", val, path, error, false);
                obs.y = static_cast<float>(val);
            }
            if (j.contains("spawnId")) {
                readString(j, "spawnId", obs.spawnId, path, error, false);
            } else {
                obs.spawnId = out.id;
            }

            if (j.contains("margin")) {
                readNumber(j, "margin", val, path, error, false);
                obs.margin = static_cast<float>(val);
            }
            if (j.contains("speedX")) {
                readNumber(j, "speedX", val, path, error, false);
                obs.speedX = static_cast<float>(val);
            }
            if (j.contains("speedY")) {
                readNumber(j, "speedY", val, path, error, false);
                obs.speedY = static_cast<float>(val);
            }
            if (j.contains("health")) {
                std::int32_t h = 0;
                readInt(j, "health", h, path, error, false);
                obs.health = h;
            }
            if (j.contains("anchor")) {
                std::string aStr;
                readString(j, "anchor", aStr, path, error, false);
                if (aStr == "top")
                    obs.anchor = ObstacleAnchor::Top;
                else if (aStr == "bottom")
                    obs.anchor = ObstacleAnchor::Bottom;
                else
                    obs.anchor = ObstacleAnchor::Absolute;
            }
            if (j.contains("scale")) {
                Vec2f s{};
                parseVec2(j["scale"], s, path, error);
                obs.scale = s;
            }

            out.obstacle = obs;
        }
        if (out.type == EventType::SpawnBoss) {
            SpawnBossSettings boss;
            if (!readString(j, "bossId", boss.bossId, path, error, true))
                return false;
            if (j.contains("spawnId")) {
                readString(j, "spawnId", boss.spawnId, path, error, false);
            } else {
                boss.spawnId = out.id;
            }
            if (!j.contains("spawn")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing spawn", "", joinPath(path, "spawn"));
                return false;
            }
            if (!parseVec2ObjectOrArray(j["spawn"], boss.spawn, joinPath(path, "spawn"), error))
                return false;
            out.boss = boss;
        }
        return true;
    }

    bool parsePatterns(const Json& j, std::vector<PatternDefinition>& out, const std::string& path,
                       LevelLoadError& error)
    {
        if (!j.isArray()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected patterns array", "", path);
            return false;
        }
        out.clear();
        out.reserve(j.size());
        for (std::size_t i = 0; i < j.size(); ++i) {
            Json p            = j[i];
            std::string ppath = joinPath(path, std::to_string(i));
            if (!p.isObject()) {
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
                readNumber(p, "speed", speed, ppath, error, true);
                out.push_back(PatternDefinition{id, MovementComponent::linear(static_cast<float>(speed))});
            } else if (type == "zigzag") {
                double speed = 0.0, amp = 0.0, freq = 0.0;
                readNumber(p, "speed", speed, ppath, error, true);
                readNumber(p, "amplitude", amp, ppath, error, true);
                readNumber(p, "frequency", freq, ppath, error, true);
                out.push_back(
                    PatternDefinition{id, MovementComponent::zigzag(static_cast<float>(speed), static_cast<float>(amp),
                                                                    static_cast<float>(freq))});
            } else if (type == "sine") {
                double speed = 0.0, amp = 0.0, freq = 0.0, phase = 0.0;
                readNumber(p, "speed", speed, ppath, error, false);
                readNumber(p, "amplitude", amp, ppath, error, true);
                readNumber(p, "frequency", freq, ppath, error, true);
                readNumber(p, "phase", phase, ppath, error, false);
                out.push_back(PatternDefinition{
                    id, MovementComponent::sine(static_cast<float>(speed), static_cast<float>(amp),
                                                static_cast<float>(freq), static_cast<float>(phase))});
            } else {
                setError(error, LevelLoadErrorCode::SchemaError, "Unknown pattern type: " + type, "", ppath);
                return false;
            }
        }
        return true;
    }

    bool parseTemplates(const Json& j, LevelTemplates& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected templates object", "", path);
            return false;
        }

        if (j.contains("hitboxes")) {
            Json hitboxes = j["hitboxes"];
            if (hitboxes.isObject()) {
                std::vector<std::string> keys = hitboxes.getKeys();
                for (const auto& key : keys) {
                    Json val = hitboxes[key];
                    HitboxComponent hb;
                    if (parseHitbox(val, hb, joinPath(path, "hitboxes/" + key), error)) {
                        out.hitboxes[key] = hb;
                    }
                }
            }
        }

        if (j.contains("colliders")) {
            Json colliders = j["colliders"];
            if (colliders.isObject()) {
                std::vector<std::string> keys = colliders.getKeys();
                for (const auto& key : keys) {
                    Json val = colliders[key];
                    ColliderComponent cc;
                    if (parseCollider(val, cc, joinPath(path, "colliders/" + key), error)) {
                        out.colliders[key] = cc;
                    }
                }
            }
        }

        if (j.contains("enemies")) {
            Json enemies = j["enemies"];
            if (enemies.isObject()) {
                std::vector<std::string> keys = enemies.getKeys();
                for (const auto& key : keys) {
                    Json e = enemies[key];
                    EnemyTemplate et;
                    int typeId = 0;
                    readInt(e, "typeId", typeId, path, error, true);
                    et.typeId = typeId;

                    std::string hbKey, colKey;
                    if (readString(e, "hitbox", hbKey, path, error, true)) {
                        if (out.hitboxes.count(hbKey))
                            et.hitbox = out.hitboxes.at(hbKey);
                        else
                            setError(error, LevelLoadErrorCode::SchemaError, "Unknown hitbox: " + hbKey, "", path);
                    }
                    if (readString(e, "collider", colKey, path, error, true)) {
                        if (out.colliders.count(colKey))
                            et.collider = out.colliders.at(colKey);
                        else
                            setError(error, LevelLoadErrorCode::SchemaError, "Unknown collider: " + colKey, "", path);
                    }
                    readInt(e, "health", et.health, path, error, false);
                    readInt(e, "score", et.score, path, error, false);

                    if (e.contains("scale")) {
                        parseVec2(e["scale"], et.scale, path, error);
                    }

                    if (e.contains("shooting")) {
                        EnemyShootingComponent shoot;
                        if (parseShooting(e["shooting"], shoot, path, error)) {
                            et.shooting = shoot;
                        }
                    }

                    out.enemies[key] = et;
                }
            }
        }

        if (j.contains("obstacles")) {
            Json obstacles = j["obstacles"];
            if (obstacles.isObject()) {
                std::vector<std::string> keys = obstacles.getKeys();
                for (const auto& key : keys) {
                    Json o = obstacles[key];
                    ObstacleTemplate ot;

                    int typeId = 0;
                    readInt(o, "typeId", typeId, path, error, true);
                    ot.typeId = typeId;

                    std::string hbKey, colKey;
                    if (readString(o, "hitbox", hbKey, path, error, true)) {
                        if (out.hitboxes.count(hbKey))
                            ot.hitbox = out.hitboxes.at(hbKey);
                        else
                            setError(error, LevelLoadErrorCode::SchemaError, "Unknown hitbox: " + hbKey, "", path);
                    }
                    if (readString(o, "collider", colKey, path, error, true)) {
                        if (out.colliders.count(colKey))
                            ot.collider = out.colliders.at(colKey);
                        else
                            setError(error, LevelLoadErrorCode::SchemaError, "Unknown collider: " + colKey, "", path);
                    }

                    readInt(o, "health", ot.health, path, error, false);

                    std::string anchorStr;
                    readString(o, "anchor", anchorStr, path, error, false);
                    if (anchorStr == "top")
                        ot.anchor = ObstacleAnchor::Top;
                    else if (anchorStr == "bottom")
                        ot.anchor = ObstacleAnchor::Bottom;
                    else
                        ot.anchor = ObstacleAnchor::Absolute;

                    double margin = 0, sx = 0, sy = 0;
                    readNumber(o, "margin", margin, path, error, false);
                    ot.margin = static_cast<float>(margin);
                    readNumber(o, "speedX", sx, path, error, false);
                    ot.speedX = static_cast<float>(sx);
                    readNumber(o, "speedY", sy, path, error, false);
                    ot.speedY = static_cast<float>(sy);

                    if (o.contains("scale")) {
                        parseVec2(o["scale"], ot.scale, path, error);
                    }

                    out.obstacles[key] = ot;
                }
            }
        }

        return true;
    }

    bool parseBosses(const Json& j, const LevelTemplates& templates,
                     std::unordered_map<std::string, BossDefinition>& out, const std::string& path,
                     LevelLoadError& error)
    {
        if (!j.isObject()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected bosses object", "", path);
            return false;
        }
        out.clear();
        std::vector<std::string> keys = j.getKeys();
        for (const auto& key : keys) {
            Json b            = j[key];
            std::string bpath = joinPath(path, key);
            if (!b.isObject()) {
                setError(error, LevelLoadErrorCode::SchemaError, "Invalid boss object", "", bpath);
                return false;
            }
            std::int32_t typeId = 0;
            std::string hitboxId;
            std::string colliderId;
            std::int32_t health = 0;
            std::int32_t score  = 0;
            if (!readInt(b, "typeId", typeId, bpath, error, true))
                return false;
            if (!readString(b, "hitbox", hitboxId, bpath, error, true))
                return false;
            if (!readString(b, "collider", colliderId, bpath, error, true))
                return false;
            if (!readInt(b, "health", health, bpath, error, true))
                return false;
            readInt(b, "score", score, bpath, error, false);
            if (!b.contains("scale")) {
                setError(error, LevelLoadErrorCode::SchemaError, "Missing scale", "", joinPath(bpath, "scale"));
                return false;
            }
            Vec2f scale{};
            if (!parseVec2(b["scale"], scale, joinPath(bpath, "scale"), error))
                return false;
            std::string patternId;
            readString(b, "patternId", patternId, bpath, error, false);

            BossDefinition boss{};
            boss.typeId = static_cast<std::uint16_t>(typeId);
            boss.health = health;
            boss.score  = std::max(0, score);
            boss.scale  = scale;
            if (!patternId.empty()) {
                boss.patternId = patternId;
            }
            auto hbIt = templates.hitboxes.find(hitboxId);
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
            if (b.contains("shooting")) {
                EnemyShootingComponent shooting{};
                if (!parseShooting(b["shooting"], shooting, joinPath(bpath, "shooting"), error))
                    return false;
                boss.shooting = shooting;
            }

            if (b.contains("phases")) {
                Json phases = b["phases"];
                if (!phases.isArray()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid phases", "", joinPath(bpath, "phases"));
                    return false;
                }
                for (std::size_t i = 0; i < phases.size(); ++i) {
                    Json p            = phases[i];
                    std::string ppath = joinPath(joinPath(bpath, "phases"), std::to_string(i));
                    if (!p.isObject()) {
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
                    if (!parseTrigger(p["trigger"], phase.trigger, joinPath(ppath, "trigger"), error))
                        return false;
                    if (!p.contains("events") || !p["events"].isArray()) {
                        setError(error, LevelLoadErrorCode::SchemaError, "Missing events", "",
                                 joinPath(ppath, "events"));
                        return false;
                    }
                    Json evs = p["events"];
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
                Json onDeath = b["onDeath"];
                if (!onDeath.isArray()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid onDeath", "", joinPath(bpath, "onDeath"));
                    return false;
                }
                const Json& evs = onDeath;
                for (std::size_t e = 0; e < evs.size(); ++e) {
                    LevelEvent ev;
                    if (!parseEvent(evs[e], ev, joinPath(joinPath(bpath, "onDeath"), std::to_string(e)), error))
                        return false;
                    boss.onDeath.push_back(std::move(ev));
                }
            }
            out[key] = std::move(boss);
        }
        return true;
    }

    bool parseSegments(const Json& j, std::vector<LevelSegment>& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isArray()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected segments array", "", path);
            return false;
        }
        out.clear();
        out.reserve(j.size());
        for (std::size_t i = 0; i < j.size(); ++i) {
            Json s            = j[i];
            std::string spath = joinPath(path, std::to_string(i));
            if (!s.isObject()) {
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
            if (!parseScroll(s["scroll"], seg.scroll, joinPath(spath, "scroll"), error))
                return false;
            if (s.contains("events")) {
                Json evs = s["events"];
                if (!evs.isArray()) {
                    setError(error, LevelLoadErrorCode::SchemaError, "Invalid events", "", joinPath(spath, "events"));
                    return false;
                }
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
            if (!parseTrigger(s["exit"], seg.exit, joinPath(spath, "exit"), error))
                return false;
            readBool(s, "bossRoom", seg.bossRoom, spath, error, false);
            if (s.contains("cameraBounds")) {
                CameraBounds bounds;
                if (!parseBounds(s["cameraBounds"], bounds, joinPath(spath, "cameraBounds"), error))
                    return false;
                seg.cameraBounds = bounds;
            }
            out.push_back(std::move(seg));
        }
        return true;
    }

    bool parseArchetypes(const Json& j, std::vector<LevelArchetype>& out, const std::string& path,
                         LevelLoadError& error)
    {
        if (!j.isArray()) {
            setError(error, LevelLoadErrorCode::SchemaError, "Expected archetypes array", "", path);
            return false;
        }
        out.clear();
        out.reserve(j.size());
        for (std::size_t i = 0; i < j.size(); ++i) {
            Json a            = j[i];
            std::string apath = joinPath(path, std::to_string(i));
            if (!a.isObject()) {
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

    bool parseMeta(const Json& j, LevelMeta& out, const std::string& path, LevelLoadError& error)
    {
        if (!j.isObject()) {
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

        for (const auto& [id, boss] : data.bosses) {
            if (!boss.patternId.has_value()) {
                continue;
            }
            bool patternFound = false;
            for (const auto& pattern : data.patterns) {
                if (pattern.id == *boss.patternId) {
                    patternFound = true;
                    break;
                }
            }
            if (!patternFound) {
                setError(error, LevelLoadErrorCode::SemanticError,
                         "Unknown patternId for boss " + id + ": " + *boss.patternId, "", "");
                return false;
            }
        }

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

    bool parseLevel(const Json& root, LevelData& out, LevelLoadError& error, const std::string& path)
    {
        if (!root.isObject()) {
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

        Json meta;
        Json archetypes;
        Json patterns;
        Json templates;
        Json segments;

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
        if (!parseMeta(meta, metaData, "/meta", error))
            return false;

        std::vector<LevelArchetype> archetypeData;
        if (!parseArchetypes(archetypes, archetypeData, "/archetypes", error))
            return false;

        std::vector<PatternDefinition> patternData;
        if (!parsePatterns(patterns, patternData, "/patterns", error))
            return false;

        LevelTemplates templateData;
        if (!parseTemplates(templates, templateData, "/templates", error))
            return false;

        std::unordered_map<std::string, BossDefinition> bosses;
        if (root.contains("bosses")) {
            Json b = root["bosses"];
            if (!parseBosses(b, templateData, bosses, "/bosses", error))
                return false;
        }

        std::vector<LevelSegment> segmentData;
        if (!parseSegments(segments, segmentData, "/segments", error))
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

    bool parseRegistry(const Json& root, LevelRegistry& out, LevelLoadError& error)
    {
        if (!root.isObject()) {
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
        Json levels;
        if (!requireArray(root, "levels", levels, "", error))
            return false;

        out.schemaVersion = schemaVersion;
        out.levels.clear();
        std::unordered_set<std::int32_t> ids;
        for (std::size_t i = 0; i < levels.size(); ++i) {
            Json entry        = levels[i];
            std::string epath = joinPath("/levels", std::to_string(i));
            if (!entry.isObject()) {
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
    Json doc;
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
    Json doc;
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
