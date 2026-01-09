#include "editor/LevelEditor.hpp"

#include "levels/LevelLoader.hpp"

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
using json = nlohmann::ordered_json;

void sortUnique(std::vector<std::string>& values)
{
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

int inputTextCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* str = static_cast<std::string*>(data->UserData);
        str->resize(static_cast<std::size_t>(data->BufTextLen));
        data->Buf = str->data();
    }
    return 0;
}

bool inputText(const char* label, std::string& value, ImGuiInputTextFlags flags = 0)
{
    if (value.capacity() < 64)
        value.reserve(64);
    flags |= ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputText(label, value.data(), value.capacity() + 1, flags, inputTextCallback, &value);
}

bool inputTextMultiline(const char* label, std::string& value, const ImVec2& size)
{
    if (value.capacity() < 256)
        value.reserve(256);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;
    return ImGui::InputTextMultiline(label, value.data(), value.capacity() + 1, size, flags, inputTextCallback, &value);
}

bool comboString(const char* label, std::string& value, const std::vector<std::string>& options)
{
    bool changed = false;
    const char* preview = value.empty() ? "<empty>" : value.c_str();
    if (ImGui::BeginCombo(label, preview)) {
        for (const auto& opt : options) {
            bool selected = (value == opt);
            if (ImGui::Selectable(opt.c_str(), selected)) {
                value   = opt;
                changed = true;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

json& ensureObject(json& obj, const char* key)
{
    if (!obj.contains(key) || !obj[key].is_object())
        obj[key] = json::object();
    return obj[key];
}

json& ensureArray(json& obj, const char* key)
{
    if (!obj.contains(key) || !obj[key].is_array())
        obj[key] = json::array();
    return obj[key];
}

json& ensureVec2(json& obj, const char* key, float x, float y)
{
    if (!obj.contains(key) || !obj[key].is_array() || obj[key].size() != 2) {
        obj[key] = json::array({x, y});
    }
    return obj[key];
}

std::vector<std::string> objectKeys(const json& obj)
{
    std::vector<std::string> keys;
    if (!obj.is_object())
        return keys;
    keys.reserve(obj.size());
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        keys.push_back(it.key());
    }
    return keys;
}

bool renameKey(json& obj, const std::string& from, const std::string& to)
{
    if (from == to || to.empty())
        return false;
    if (obj.contains(to))
        return false;
    obj[to] = obj[from];
    obj.erase(from);
    return true;
}

std::string uniqueId(const std::string& base, const std::vector<std::string>& existing)
{
    if (std::find(existing.begin(), existing.end(), base) == existing.end())
        return base;
    int suffix = 1;
    while (true) {
        std::string candidate = base + "_" + std::to_string(suffix);
        if (std::find(existing.begin(), existing.end(), candidate) == existing.end())
            return candidate;
        ++suffix;
    }
}

json makeDefaultLevel()
{
    json root = json::object();
    root["schemaVersion"] = 1;
    root["levelId"]       = 1;
    root["meta"]          = json::object({{"name", ""}, {"backgroundId", ""}, {"musicId", ""}, {"author", ""},
                                 {"difficulty", ""}});
    root["archetypes"]    = json::array();
    root["patterns"]      = json::array();
    root["templates"]     = json::object(
        {{"hitboxes", json::object()}, {"colliders", json::object()}, {"enemies", json::object()},
         {"obstacles", json::object()}});
    root["segments"] = json::array();
    return root;
}

json makeBaseLevel(const AssetIndex& assets)
{
    json root = makeDefaultLevel();

    root["meta"]["backgroundId"] = assets.backgrounds.empty() ? "" : assets.backgrounds.front();
    root["meta"]["musicId"]      = assets.music.empty() ? "" : assets.music.front();

    root["archetypes"] = json::array(
        {json::object({{"typeId", 1}, {"spriteId", "player_ship"}, {"animId", "player1"}, {"layer", 0}}),
         json::object({{"typeId", 12}, {"spriteId", "player_ship"}, {"animId", "player2"}, {"layer", 0}}),
         json::object({{"typeId", 13}, {"spriteId", "player_ship"}, {"animId", "player3"}, {"layer", 0}}),
         json::object({{"typeId", 14}, {"spriteId", "player_ship"}, {"animId", "player4"}, {"layer", 0}}),
         json::object({{"typeId", 3}, {"spriteId", "bullet"}, {"animId", "bullet_basic"}, {"layer", 0}}),
         json::object({{"typeId", 4}, {"spriteId", "bullet"}, {"animId", "bullet_charge_lvl1"}, {"layer", 0}}),
         json::object({{"typeId", 5}, {"spriteId", "bullet"}, {"animId", "bullet_charge_lvl2"}, {"layer", 0}}),
         json::object({{"typeId", 6}, {"spriteId", "bullet"}, {"animId", "bullet_charge_lvl3"}, {"layer", 0}}),
         json::object({{"typeId", 7}, {"spriteId", "bullet"}, {"animId", "bullet_charge_lvl4"}, {"layer", 0}}),
         json::object({{"typeId", 8}, {"spriteId", "bullet"}, {"animId", "bullet_charge_lvl5"}, {"layer", 0}}),
         json::object({{"typeId", 15}, {"spriteId", "enemy_bullet"}, {"animId", "enemy_bullet_basic"}, {"layer", 0}}),
         json::object({{"typeId", 16}, {"spriteId", "bullet"}, {"animId", "player_death"}, {"layer", 50}}),
         json::object({{"typeId", 22}, {"spriteId", "walker_special_shot"}, {"animId", "walker_special_shot"},
                       {"layer", 1}}),
         json::object({{"typeId", 2}, {"spriteId", "mob1"}, {"animId", "left"}, {"layer", 0}}),
         json::object({{"typeId", 21}, {"spriteId", "mob2"}, {"animId", "walking"}, {"layer", 0}}),
         json::object({{"typeId", 9}, {"spriteId", "obstacle_top"}, {"animId", ""}, {"layer", 0}}),
         json::object({{"typeId", 10}, {"spriteId", "obstacle_middle"}, {"animId", ""}, {"layer", 0}}),
         json::object({{"typeId", 11}, {"spriteId", "obstacle_bottom"}, {"animId", ""}, {"layer", 0}}),
         json::object({{"typeId", 20}, {"spriteId", "enemy_ship"}, {"animId", "enemy1"}, {"layer", 0}})});

    root["patterns"] = json::array({json::object({{"id", "p_linear"}, {"type", "linear"}, {"speed", 140}}),
                                    json::object({{"id", "p_sine"},
                                                  {"type", "sine"},
                                                  {"speed", 120},
                                                  {"amplitude", 140},
                                                  {"frequency", 0.6},
                                                  {"phase", 0.0}}),
                                    json::object({{"id", "p_zigzag"},
                                                  {"type", "zigzag"},
                                                  {"speed", 150},
                                                  {"amplitude", 120},
                                                  {"frequency", 0.9}}),
                                    json::object({{"id", "p_boss_hover"},
                                                  {"type", "sine"},
                                                  {"speed", 0},
                                                  {"amplitude", 90},
                                                  {"frequency", 0.35},
                                                  {"phase", 0.0}})});

    root["templates"]["hitboxes"] = json::object(
        {{"enemy_small", json::object({{"width", 48}, {"height", 36}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"enemy_walker", json::object({{"width", 32}, {"height", 32}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"ob_top", json::object({{"width", 147}, {"height", 23}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"ob_mid", json::object({{"width", 105}, {"height", 47}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"ob_bot", json::object({{"width", 146}, {"height", 40}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"boss_hit", json::object({{"width", 140}, {"height", 90}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})}});

    root["templates"]["colliders"] = json::object(
        {{"enemy_small",
          json::object({{"shape", "box"}, {"width", 48}, {"height", 36}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"enemy_walker",
          json::object({{"shape", "box"}, {"width", 32}, {"height", 32}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})},
         {"ob_top",
          json::object({{"shape", "polygon"},
                        {"offsetX", 0},
                        {"offsetY", 0},
                        {"active", true},
                        {"points",
                         json::array({json::array({0, 0}),
                                      json::array({146, 0}),
                                      json::array({146, 4}),
                                      json::array({144, 7}),
                                      json::array({139, 14}),
                                      json::array({137, 16}),
                                      json::array({129, 22}),
                                      json::array({24, 22}),
                                      json::array({4, 6}),
                                      json::array({0, 2})})}})},
         {"ob_mid",
          json::object({{"shape", "polygon"},
                        {"offsetX", 0},
                        {"offsetY", 0},
                        {"active", true},
                        {"points",
                         json::array({json::array({0, 24}),
                                      json::array({2, 20}),
                                      json::array({8, 10}),
                                      json::array({10, 8}),
                                      json::array({19, 2}),
                                      json::array({21, 1}),
                                      json::array({72, 1}),
                                      json::array({90, 6}),
                                      json::array({93, 7}),
                                      json::array({101, 11}),
                                      json::array({104, 14}),
                                      json::array({104, 46}),
                                      json::array({21, 46}),
                                      json::array({19, 45}),
                                      json::array({11, 39}),
                                      json::array({1, 29}),
                                      json::array({0, 27})})}})},
         {"ob_bot",
          json::object({{"shape", "polygon"},
                        {"offsetX", 0},
                        {"offsetY", 0},
                        {"active", true},
                        {"points",
                         json::array({json::array({0, 35}),
                                      json::array({1, 33}),
                                      json::array({6, 26}),
                                      json::array({8, 24}),
                                      json::array({16, 18}),
                                      json::array({18, 17}),
                                      json::array({71, 0}),
                                      json::array({80, 0}),
                                      json::array({83, 1}),
                                      json::array({119, 17}),
                                      json::array({125, 21}),
                                      json::array({138, 30}),
                                      json::array({143, 34}),
                                      json::array({145, 39}),
                                      json::array({0, 39})})}})},
         {"boss_box",
          json::object({{"shape", "box"}, {"width", 140}, {"height", 90}, {"offsetX", 0}, {"offsetY", 0}, {"active", true}})}});

    root["templates"]["enemies"] = json::object(
        {{"grunt",
          json::object({{"typeId", 2},
                        {"hitbox", "enemy_small"},
                        {"collider", "enemy_small"},
                        {"health", 2},
                        {"score", 200},
                        {"scale", json::array({1.4, 1.4})},
                        {"shooting", json::object({{"interval", 1.6}, {"speed", 260}, {"damage", 6}, {"lifetime", 3.0}})}})},
         {"walker",
          json::object({{"typeId", 21},
                        {"hitbox", "enemy_walker"},
                        {"collider", "enemy_walker"},
                        {"health", 2},
                        {"score", 220},
                        {"scale", json::array({1.4, 1.4})},
                        {"shooting", json::object({{"interval", 2.8}, {"speed", 260}, {"damage", 6}, {"lifetime", 3.0}})}})}});

    root["templates"]["obstacles"] = json::object(
        {{"wall_top",
          json::object({{"typeId", 9},
                        {"hitbox", "ob_top"},
                        {"collider", "ob_top"},
                        {"health", 20},
                        {"anchor", "top"},
                        {"margin", 0},
                        {"speedX", -60},
                        {"speedY", 0},
                        {"scale", json::array({1.4, 1.4})}})},
         {"wall_mid",
          json::object({{"typeId", 10},
                        {"hitbox", "ob_mid"},
                        {"collider", "ob_mid"},
                        {"health", 28},
                        {"anchor", "absolute"},
                        {"margin", 0},
                        {"speedX", -60},
                        {"speedY", 0},
                        {"scale", json::array({1.5, 1.5})}})},
         {"wall_bot",
          json::object({{"typeId", 11},
                        {"hitbox", "ob_bot"},
                        {"collider", "ob_bot"},
                        {"health", 22},
                        {"anchor", "bottom"},
                        {"margin", 20},
                        {"speedX", -60},
                        {"speedY", 0},
                        {"scale", json::array({1.4, 1.4})}})}});

    root["bosses"] = json::object({{"boss_alpha",
                                    json::object({{"typeId", 20},
                                                  {"hitbox", "boss_hit"},
                                                  {"collider", "boss_box"},
                                                  {"health", 80},
                                                  {"score", 1000},
                                                  {"scale", json::array({2.5, 2.5})},
                                                  {"patternId", "p_boss_hover"},
                                                  {"shooting",
                                                   json::object({{"interval", 1.3}, {"speed", 320}, {"damage", 8}, {"lifetime", 4.0}})},
                                                  {"phases", json::array()},
                                                  {"onDeath", json::array()}})}});

    root["segments"] = json::array({json::object({{"id", "segment_1"},
                                                 {"scroll", json::object({{"mode", "constant"}, {"speedX", -60}})},
                                                 {"events", json::array()},
                                                 {"exit", json::object({{"type", "distance"}, {"distance", 800}})}})});

    return root;
}

json makeDefaultTrigger(const std::string& type)
{
    json trigger = json::object();
    trigger["type"] = type;
    if (type == "time")
        trigger["time"] = 0.0;
    else if (type == "distance")
        trigger["distance"] = 0.0;
    else if (type == "spawn_dead")
        trigger["spawnId"] = "";
    else if (type == "boss_dead")
        trigger["bossId"] = "";
    else if (type == "enemy_count_at_most")
        trigger["count"] = 0;
    else if (type == "checkpoint_reached")
        trigger["checkpointId"] = "";
    else if (type == "hp_below") {
        trigger["bossId"] = "";
        trigger["value"]  = 0;
    } else if (type == "player_in_zone") {
        trigger["bounds"] = json::object({{"minX", 0.0}, {"maxX", 0.0}, {"minY", 0.0}, {"maxY", 0.0}});
    } else if (type == "players_ready") {
    } else if (type == "all_of" || type == "any_of") {
        trigger["triggers"] = json::array();
    }
    return trigger;
}

json makeDefaultRepeat()
{
    json repeat = json::object();
    repeat["interval"] = 1.0;
    repeat["count"]    = 1;
    return repeat;
}

json makeDefaultWave(const std::string& type)
{
    json wave = json::object();
    wave["type"]      = type;
    wave["enemy"]     = "";
    wave["patternId"] = "";
    if (type == "line") {
        wave["spawnX"] = 0.0;
        wave["startY"] = 0.0;
        wave["deltaY"] = 0.0;
        wave["count"]  = 1;
    } else if (type == "stagger") {
        wave["spawnX"]  = 0.0;
        wave["startY"]  = 0.0;
        wave["deltaY"]  = 0.0;
        wave["count"]   = 1;
        wave["spacing"] = 0.2;
    } else if (type == "triangle") {
        wave["spawnX"]        = 0.0;
        wave["apexY"]         = 0.0;
        wave["rowHeight"]     = 40.0;
        wave["horizontalStep"] = 40.0;
        wave["layers"]        = 2;
    } else if (type == "serpent") {
        wave["spawnX"]     = 0.0;
        wave["startY"]     = 0.0;
        wave["stepY"]      = 60.0;
        wave["count"]      = 4;
        wave["amplitudeX"] = 80.0;
        wave["stepTime"]   = 0.4;
    } else if (type == "cross") {
        wave["centerX"]  = 0.0;
        wave["centerY"]  = 0.0;
        wave["step"]     = 40.0;
        wave["armLength"] = 2;
    }
    return wave;
}

json makeDefaultEvent(const std::string& type)
{
    json ev = json::object();
    ev["type"] = type;
    ev["id"]   = "";
    ev["trigger"] = makeDefaultTrigger("time");
    if (type == "spawn_wave") {
        ev["wave"] = makeDefaultWave("line");
    } else if (type == "spawn_obstacle") {
        ev["obstacle"] = "";
        ev["x"]        = 0.0;
    } else if (type == "spawn_boss") {
        ev["bossId"] = "";
        ev["spawn"]  = json::object({{"x", 0.0}, {"y", 0.0}});
    } else if (type == "set_scroll") {
        ev["scroll"] = json::object({{"mode", "constant"}, {"speedX", 0.0}});
    } else if (type == "set_background") {
        ev["backgroundId"] = "";
    } else if (type == "set_music") {
        ev["musicId"] = "";
    } else if (type == "set_camera_bounds") {
        ev["bounds"] = json::object({{"minX", 0.0}, {"maxX", 0.0}, {"minY", 0.0}, {"maxY", 0.0}});
    } else if (type == "set_player_bounds") {
        ev["bounds"] = json::object({{"minX", 0.0}, {"maxX", 0.0}, {"minY", 0.0}, {"maxY", 0.0}});
    } else if (type == "clear_player_bounds") {
    } else if (type == "gate_open" || type == "gate_close") {
        ev["gateId"] = "";
    } else if (type == "checkpoint") {
        ev.erase("id");
        ev["checkpointId"] = "";
        ev["respawn"]      = json::object({{"x", 0.0}, {"y", 0.0}});
    }
    return ev;
}

json makeDefaultHitbox()
{
    return json::object({{"width", 0.0}, {"height", 0.0}, {"offsetX", 0.0}, {"offsetY", 0.0}, {"active", true}});
}

json makeDefaultCollider()
{
    return json::object({{"shape", "box"}, {"width", 0.0}, {"height", 0.0}, {"offsetX", 0.0}, {"offsetY", 0.0},
                         {"active", true}});
}

json makeDefaultEnemy()
{
    return json::object({{"typeId", 0}, {"hitbox", ""}, {"collider", ""}, {"health", 1}, {"score", 0},
                         {"scale", json::array({1.0, 1.0})}});
}

json makeDefaultObstacle()
{
    return json::object({{"typeId", 0}, {"hitbox", ""}, {"collider", ""}, {"health", 1}, {"anchor", "absolute"},
                         {"margin", 0.0}, {"speedX", 0.0}, {"speedY", 0.0}, {"scale", json::array({1.0, 1.0})}});
}

json makeDefaultBoss()
{
    return json::object({{"typeId", 0}, {"hitbox", ""}, {"collider", ""}, {"health", 1}, {"score", 0},
                         {"scale", json::array({1.0, 1.0})}});
}

json makeDefaultSegment()
{
    json seg = json::object();
    seg["id"]    = "segment";
    seg["scroll"] = json::object({{"mode", "constant"}, {"speedX", -60.0}});
    seg["events"] = json::array();
    seg["exit"]   = makeDefaultTrigger("distance");
    return seg;
}

bool drawVec2(json& obj, const char* key, const char* label, float defaultX, float defaultY)
{
    json& vec = ensureVec2(obj, key, defaultX, defaultY);
    float v[2] = {vec[0].get<float>(), vec[1].get<float>()};
    if (ImGui::DragFloat2(label, v, 0.1f)) {
        vec[0] = v[0];
        vec[1] = v[1];
        return true;
    }
    return false;
}

bool drawOptionalVec2(json& obj, const char* key, const char* label, float defaultX, float defaultY)
{
    bool enabled = obj.contains(key);
    bool changed = false;
    ImGui::PushID(label);
    if (ImGui::Checkbox("##enabled", &enabled)) {
        if (enabled) {
            obj[key] = json::array({defaultX, defaultY});
        } else {
            obj.erase(key);
        }
        changed = true;
    }
    ImGui::SameLine();
    if (enabled) {
        changed = drawVec2(obj, key, label, defaultX, defaultY) || changed;
    } else {
        ImGui::TextDisabled("%s", label);
    }
    ImGui::PopID();
    return changed;
}

bool drawOptionalFloat(json& obj, const char* key, const char* label, float defaultValue, float speed = 0.1f)
{
    bool enabled = obj.contains(key);
    bool changed = false;
    ImGui::PushID(label);
    if (ImGui::Checkbox("##enabled", &enabled)) {
        if (enabled)
            obj[key] = defaultValue;
        else
            obj.erase(key);
        changed = true;
    }
    ImGui::SameLine();
    if (enabled) {
        float v = obj[key].get<float>();
        if (ImGui::DragFloat(label, &v, speed)) {
            obj[key] = v;
            changed  = true;
        }
    } else {
        ImGui::TextDisabled("%s", label);
    }
    ImGui::PopID();
    return changed;
}

bool drawOptionalInt(json& obj, const char* key, const char* label, int defaultValue)
{
    bool enabled = obj.contains(key);
    bool changed = false;
    ImGui::PushID(label);
    if (ImGui::Checkbox("##enabled", &enabled)) {
        if (enabled)
            obj[key] = defaultValue;
        else
            obj.erase(key);
        changed = true;
    }
    ImGui::SameLine();
    if (enabled) {
        int v = obj[key].get<int>();
        if (ImGui::DragInt(label, &v)) {
            obj[key] = v;
            changed  = true;
        }
    } else {
        ImGui::TextDisabled("%s", label);
    }
    ImGui::PopID();
    return changed;
}

bool drawOptionalBool(json& obj, const char* key, const char* label, bool defaultValue)
{
    bool enabled = obj.contains(key);
    bool changed = false;
    ImGui::PushID(label);
    if (ImGui::Checkbox("##enabled", &enabled)) {
        if (enabled)
            obj[key] = defaultValue;
        else
            obj.erase(key);
        changed = true;
    }
    ImGui::SameLine();
    if (enabled) {
        bool v = obj[key].get<bool>();
        if (ImGui::Checkbox(label, &v)) {
            obj[key] = v;
            changed  = true;
        }
    } else {
        ImGui::TextDisabled("%s", label);
    }
    ImGui::PopID();
    return changed;
}

bool drawOptionalString(json& obj, const char* key, const char* label, const std::vector<std::string>& options)
{
    bool enabled = obj.contains(key);
    bool changed = false;
    ImGui::PushID(label);
    if (ImGui::Checkbox("##enabled", &enabled)) {
        if (enabled)
            obj[key] = "";
        else
            obj.erase(key);
        changed = true;
    }
    ImGui::SameLine();
    if (enabled) {
        std::string v = obj[key].get<std::string>();
        if (comboString(label, v, options)) {
            obj[key] = v;
            changed  = true;
        }
        if (inputText("##value", v)) {
            obj[key] = v;
            changed  = true;
        }
    } else {
        ImGui::TextDisabled("%s", label);
    }
    ImGui::PopID();
    return changed;
}

std::vector<std::string> mergeAnimationOptions(const AssetIndex& assets, const std::string& spriteId)
{
    std::vector<std::string> result = assets.animations;
    auto it = assets.labels.find(spriteId);
    if (it != assets.labels.end()) {
        result.insert(result.end(), it->second.begin(), it->second.end());
    }
    sortUnique(result);
    return result;
}

void drawTrigger(json& trigger, const IdCache& ids, bool& changed);
void drawEvents(json& events, const IdCache& ids, const AssetIndex& assets, bool& changed);

void drawTrigger(json& trigger, const IdCache& ids, bool& changed)
{
    if (!trigger.is_object())
        trigger = makeDefaultTrigger("time");

    std::string type = trigger.value("type", "time");
    const std::vector<std::string> types = {"time", "distance", "spawn_dead", "boss_dead", "enemy_count_at_most",
                                            "checkpoint_reached", "hp_below", "player_in_zone", "players_ready",
                                            "all_of", "any_of"};

    if (comboString("Type", type, types)) {
        trigger = makeDefaultTrigger(type);
        changed = true;
    }

    if (type == "time") {
        double val = trigger.value("time", 0.0);
        float v    = static_cast<float>(val);
        if (ImGui::DragFloat("Time", &v, 0.1f)) {
            trigger["time"] = v;
            changed         = true;
        }
    } else if (type == "distance") {
        double val = trigger.value("distance", 0.0);
        float v    = static_cast<float>(val);
        if (ImGui::DragFloat("Distance", &v, 1.0f)) {
            trigger["distance"] = v;
            changed              = true;
        }
    } else if (type == "spawn_dead") {
        std::string id = trigger.value("spawnId", "");
        if (comboString("SpawnId", id, ids.spawnIds)) {
            trigger["spawnId"] = id;
            changed             = true;
        }
        if (inputText("##spawnId", id)) {
            trigger["spawnId"] = id;
            changed             = true;
        }
    } else if (type == "boss_dead") {
        std::string id = trigger.value("bossId", "");
        if (comboString("BossId", id, ids.bossIds)) {
            trigger["bossId"] = id;
            changed            = true;
        }
        if (inputText("##bossId", id)) {
            trigger["bossId"] = id;
            changed            = true;
        }
    } else if (type == "enemy_count_at_most") {
        int count = trigger.value("count", 0);
        if (ImGui::DragInt("Count", &count)) {
            trigger["count"] = count;
            changed           = true;
        }
    } else if (type == "checkpoint_reached") {
        std::string id = trigger.value("checkpointId", "");
        if (comboString("CheckpointId", id, ids.checkpointIds)) {
            trigger["checkpointId"] = id;
            changed                  = true;
        }
        if (inputText("##checkpointId", id)) {
            trigger["checkpointId"] = id;
            changed                  = true;
        }
    } else if (type == "hp_below") {
        std::string id = trigger.value("bossId", "");
        if (comboString("BossId", id, ids.bossIds)) {
            trigger["bossId"] = id;
            changed            = true;
        }
        if (inputText("##bossId", id)) {
            trigger["bossId"] = id;
            changed            = true;
        }
        int value = trigger.value("value", 0);
        if (ImGui::DragInt("HP", &value)) {
            trigger["value"] = value;
            changed           = true;
        }
    } else if (type == "player_in_zone") {
        json& bounds = ensureObject(trigger, "bounds");
        float minX   = static_cast<float>(bounds.value("minX", 0.0));
        float maxX   = static_cast<float>(bounds.value("maxX", 0.0));
        float minY   = static_cast<float>(bounds.value("minY", 0.0));
        float maxY   = static_cast<float>(bounds.value("maxY", 0.0));
        if (ImGui::DragFloat("MinX", &minX, 1.0f)) {
            bounds["minX"] = minX;
            changed         = true;
        }
        if (ImGui::DragFloat("MaxX", &maxX, 1.0f)) {
            bounds["maxX"] = maxX;
            changed         = true;
        }
        if (ImGui::DragFloat("MinY", &minY, 1.0f)) {
            bounds["minY"] = minY;
            changed         = true;
        }
        if (ImGui::DragFloat("MaxY", &maxY, 1.0f)) {
            bounds["maxY"] = maxY;
            changed         = true;
        }
        bool requireAll = trigger.value("requireAll", false);
        if (ImGui::Checkbox("RequireAll", &requireAll)) {
            trigger["requireAll"] = requireAll;
            changed                = true;
        }
    } else if (type == "players_ready") {
    } else if (type == "all_of" || type == "any_of") {
        json& children = ensureArray(trigger, "triggers");
        if (ImGui::Button("Add trigger")) {
            children.push_back(makeDefaultTrigger("time"));
            changed = true;
        }
        for (std::size_t i = 0; i < children.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::CollapsingHeader("Trigger", ImGuiTreeNodeFlags_DefaultOpen)) {
                drawTrigger(children[i], ids, changed);
                if (ImGui::Button("Remove")) {
                    children.erase(children.begin() + static_cast<long>(i));
                    changed = true;
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
    }
}

void drawWave(json& wave, bool& changed, const std::vector<std::string>& enemyIds,
              const std::vector<std::string>& patternIds)
{
    if (!wave.is_object())
        wave = makeDefaultWave("line");

    std::string type = wave.value("type", "line");
    const std::vector<std::string> types = {"line", "stagger", "triangle", "serpent", "cross"};
    if (comboString("WaveType", type, types)) {
        json newWave = makeDefaultWave(type);
        newWave["enemy"]     = wave.value("enemy", "");
        newWave["patternId"] = wave.value("patternId", "");
        wave                 = newWave;
        changed              = true;
    }

    std::string enemy = wave.value("enemy", "");
    if (comboString("Enemy", enemy, enemyIds)) {
        wave["enemy"] = enemy;
        changed       = true;
    }
    if (inputText("##enemy", enemy)) {
        wave["enemy"] = enemy;
        changed       = true;
    }

    std::string pattern = wave.value("patternId", "");
    if (comboString("Pattern", pattern, patternIds)) {
        wave["patternId"] = pattern;
        changed            = true;
    }
    if (inputText("##pattern", pattern)) {
        wave["patternId"] = pattern;
        changed            = true;
    }

    if (type == "line") {
        float spawnX = static_cast<float>(wave.value("spawnX", 0.0));
        float startY = static_cast<float>(wave.value("startY", 0.0));
        float deltaY = static_cast<float>(wave.value("deltaY", 0.0));
        int count    = wave.value("count", 1);
        if (ImGui::DragFloat("SpawnX", &spawnX, 1.0f)) {
            wave["spawnX"] = spawnX;
            changed        = true;
        }
        if (ImGui::DragFloat("StartY", &startY, 1.0f)) {
            wave["startY"] = startY;
            changed        = true;
        }
        if (ImGui::DragFloat("DeltaY", &deltaY, 1.0f)) {
            wave["deltaY"] = deltaY;
            changed        = true;
        }
        if (ImGui::DragInt("Count", &count)) {
            wave["count"] = count;
            changed       = true;
        }
    } else if (type == "stagger") {
        float spawnX  = static_cast<float>(wave.value("spawnX", 0.0));
        float startY  = static_cast<float>(wave.value("startY", 0.0));
        float deltaY  = static_cast<float>(wave.value("deltaY", 0.0));
        int count     = wave.value("count", 1);
        float spacing = static_cast<float>(wave.value("spacing", 0.2));
        if (ImGui::DragFloat("SpawnX", &spawnX, 1.0f)) {
            wave["spawnX"] = spawnX;
            changed        = true;
        }
        if (ImGui::DragFloat("StartY", &startY, 1.0f)) {
            wave["startY"] = startY;
            changed        = true;
        }
        if (ImGui::DragFloat("DeltaY", &deltaY, 1.0f)) {
            wave["deltaY"] = deltaY;
            changed        = true;
        }
        if (ImGui::DragInt("Count", &count)) {
            wave["count"] = count;
            changed       = true;
        }
        if (ImGui::DragFloat("Spacing", &spacing, 0.01f)) {
            wave["spacing"] = spacing;
            changed          = true;
        }
    } else if (type == "triangle") {
        float spawnX         = static_cast<float>(wave.value("spawnX", 0.0));
        float apexY          = static_cast<float>(wave.value("apexY", 0.0));
        float rowHeight      = static_cast<float>(wave.value("rowHeight", 40.0));
        float horizontalStep = static_cast<float>(wave.value("horizontalStep", 40.0));
        int layers           = wave.value("layers", 2);
        if (ImGui::DragFloat("SpawnX", &spawnX, 1.0f)) {
            wave["spawnX"] = spawnX;
            changed        = true;
        }
        if (ImGui::DragFloat("ApexY", &apexY, 1.0f)) {
            wave["apexY"] = apexY;
            changed       = true;
        }
        if (ImGui::DragFloat("RowHeight", &rowHeight, 1.0f)) {
            wave["rowHeight"] = rowHeight;
            changed            = true;
        }
        if (ImGui::DragFloat("HorizontalStep", &horizontalStep, 1.0f)) {
            wave["horizontalStep"] = horizontalStep;
            changed                  = true;
        }
        if (ImGui::DragInt("Layers", &layers)) {
            wave["layers"] = layers;
            changed         = true;
        }
    } else if (type == "serpent") {
        float spawnX     = static_cast<float>(wave.value("spawnX", 0.0));
        float startY     = static_cast<float>(wave.value("startY", 0.0));
        float stepY      = static_cast<float>(wave.value("stepY", 60.0));
        int count        = wave.value("count", 4);
        float amplitudeX = static_cast<float>(wave.value("amplitudeX", 80.0));
        float stepTime   = static_cast<float>(wave.value("stepTime", 0.4));
        if (ImGui::DragFloat("SpawnX", &spawnX, 1.0f)) {
            wave["spawnX"] = spawnX;
            changed        = true;
        }
        if (ImGui::DragFloat("StartY", &startY, 1.0f)) {
            wave["startY"] = startY;
            changed        = true;
        }
        if (ImGui::DragFloat("StepY", &stepY, 1.0f)) {
            wave["stepY"] = stepY;
            changed       = true;
        }
        if (ImGui::DragInt("Count", &count)) {
            wave["count"] = count;
            changed       = true;
        }
        if (ImGui::DragFloat("AmplitudeX", &amplitudeX, 1.0f)) {
            wave["amplitudeX"] = amplitudeX;
            changed             = true;
        }
        if (ImGui::DragFloat("StepTime", &stepTime, 0.01f)) {
            wave["stepTime"] = stepTime;
            changed           = true;
        }
    } else if (type == "cross") {
        float centerX = static_cast<float>(wave.value("centerX", 0.0));
        float centerY = static_cast<float>(wave.value("centerY", 0.0));
        float step    = static_cast<float>(wave.value("step", 40.0));
        int armLength = wave.value("armLength", 2);
        if (ImGui::DragFloat("CenterX", &centerX, 1.0f)) {
            wave["centerX"] = centerX;
            changed         = true;
        }
        if (ImGui::DragFloat("CenterY", &centerY, 1.0f)) {
            wave["centerY"] = centerY;
            changed         = true;
        }
        if (ImGui::DragFloat("Step", &step, 1.0f)) {
            wave["step"] = step;
            changed      = true;
        }
        if (ImGui::DragInt("ArmLength", &armLength)) {
            wave["armLength"] = armLength;
            changed            = true;
        }
    }

    changed = drawOptionalInt(wave, "health", "Health", 1) || changed;
    changed = drawOptionalVec2(wave, "scale", "Scale", 1.0f, 1.0f) || changed;
    changed = drawOptionalBool(wave, "shootingEnabled", "Shooting", true) || changed;
}

void drawScroll(json& scroll, bool& changed)
{
    if (!scroll.is_object())
        scroll = json::object({{"mode", "constant"}, {"speedX", 0.0}});

    std::string mode = scroll.value("mode", "constant");
    const std::vector<std::string> modes = {"constant", "stopped", "curve"};
    if (comboString("Mode", mode, modes)) {
        scroll["mode"] = mode;
        if (mode == "curve") {
            scroll["curve"] = json::array({json::object({{"time", 0.0}, {"speedX", 0.0}})});
        }
        changed = true;
    }

    if (mode == "constant") {
        float speed = static_cast<float>(scroll.value("speedX", scroll.value("speed", 0.0)));
        if (ImGui::DragFloat("SpeedX", &speed, 1.0f)) {
            scroll["speedX"] = speed;
            if (scroll.contains("speed"))
                scroll["speed"] = speed;
            changed = true;
        }
    } else if (mode == "curve") {
        json& curve = ensureArray(scroll, "curve");
        if (ImGui::Button("Add key")) {
            curve.push_back(json::object({{"time", 0.0}, {"speedX", 0.0}}));
            changed = true;
        }
        for (std::size_t i = 0; i < curve.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            float time = static_cast<float>(curve[i].value("time", 0.0));
            float speed = static_cast<float>(curve[i].value("speedX", 0.0));
            if (ImGui::DragFloat("Time", &time, 0.1f)) {
                curve[i]["time"] = time;
                changed            = true;
            }
            if (ImGui::DragFloat("SpeedX", &speed, 1.0f)) {
                curve[i]["speedX"] = speed;
                changed              = true;
            }
            if (ImGui::Button("Remove")) {
                curve.erase(curve.begin() + static_cast<long>(i));
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
            ImGui::PopID();
        }
    }
}

void drawEvent(json& ev, const IdCache& ids, const AssetIndex& assets, bool& changed)
{
    if (!ev.is_object())
        ev = makeDefaultEvent("spawn_wave");

    std::string type = ev.value("type", "spawn_wave");
    const std::vector<std::string> types = {"spawn_wave",   "spawn_obstacle", "spawn_boss", "set_scroll",
                                            "set_background", "set_music",   "set_camera_bounds",
                                            "set_player_bounds", "clear_player_bounds", "gate_open",
                                            "gate_close",    "checkpoint"};

    if (comboString("EventType", type, types)) {
        json newEv = makeDefaultEvent(type);
        if (ev.contains("id"))
            newEv["id"] = ev["id"];
        if (ev.contains("trigger"))
            newEv["trigger"] = ev["trigger"];
        if (ev.contains("repeat"))
            newEv["repeat"] = ev["repeat"];
        ev      = newEv;
        changed = true;
    }

    if (ev.contains("id")) {
        std::string id = ev.value("id", "");
        if (inputText("Id", id)) {
            ev["id"] = id;
            changed  = true;
        }
    }

    if (!ev.contains("trigger"))
        ev["trigger"] = makeDefaultTrigger("time");
    if (ImGui::CollapsingHeader("Trigger", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("trigger");
        drawTrigger(ev["trigger"], ids, changed);
        ImGui::PopID();
    }

    bool repeatEnabled = ev.contains("repeat");
    if (ImGui::Checkbox("Repeat", &repeatEnabled)) {
        if (repeatEnabled)
            ev["repeat"] = makeDefaultRepeat();
        else
            ev.erase("repeat");
        changed = true;
    }
    if (ev.contains("repeat")) {
        json& repeat = ev["repeat"];
        float interval = static_cast<float>(repeat.value("interval", 1.0));
        if (ImGui::DragFloat("Interval", &interval, 0.05f)) {
            repeat["interval"] = interval;
            changed             = true;
        }
        changed = drawOptionalInt(repeat, "count", "Count", 1) || changed;
        if (repeat.contains("until")) {
            if (ImGui::CollapsingHeader("Until")) {
                drawTrigger(repeat["until"], ids, changed);
            }
            if (ImGui::Button("Remove Until")) {
                repeat.erase("until");
                changed = true;
            }
        } else {
            if (ImGui::Button("Add Until")) {
                repeat["until"] = makeDefaultTrigger("time");
                changed          = true;
            }
        }
    }

    if (type == "spawn_wave") {
        if (!ev.contains("wave"))
            ev["wave"] = makeDefaultWave("line");
        drawWave(ev["wave"], changed, ids.enemyTemplateIds, ids.patternIds);
    } else if (type == "spawn_obstacle") {
        std::string ob = ev.value("obstacle", "");
        if (comboString("Obstacle", ob, ids.obstacleTemplateIds)) {
            ev["obstacle"] = ob;
            changed         = true;
        }
        if (inputText("##obstacle", ob)) {
            ev["obstacle"] = ob;
            changed         = true;
        }
        float x = static_cast<float>(ev.value("x", 0.0));
        if (ImGui::DragFloat("X", &x, 1.0f)) {
            ev["x"] = x;
            changed  = true;
        }
        changed = drawOptionalFloat(ev, "y", "Y", 0.0f, 1.0f) || changed;
        changed = drawOptionalString(ev, "spawnId", "SpawnId", ids.spawnIds) || changed;
        const std::vector<std::string> anchors = {"absolute", "top", "bottom"};
        changed = drawOptionalString(ev, "anchor", "Anchor", anchors) || changed;
        changed = drawOptionalFloat(ev, "margin", "Margin", 0.0f, 1.0f) || changed;
        changed = drawOptionalFloat(ev, "speedX", "SpeedX", 0.0f, 1.0f) || changed;
        changed = drawOptionalFloat(ev, "speedY", "SpeedY", 0.0f, 1.0f) || changed;
        changed = drawOptionalInt(ev, "health", "Health", 1) || changed;
        changed = drawOptionalVec2(ev, "scale", "Scale", 1.0f, 1.0f) || changed;
    } else if (type == "spawn_boss") {
        std::string boss = ev.value("bossId", "");
        if (comboString("BossId", boss, ids.bossIds)) {
            ev["bossId"] = boss;
            changed       = true;
        }
        if (inputText("##bossId", boss)) {
            ev["bossId"] = boss;
            changed       = true;
        }
        changed = drawOptionalString(ev, "spawnId", "SpawnId", ids.spawnIds) || changed;
        json& spawn = ensureObject(ev, "spawn");
        float sx    = static_cast<float>(spawn.value("x", 0.0));
        float sy    = static_cast<float>(spawn.value("y", 0.0));
        if (ImGui::DragFloat("SpawnX", &sx, 1.0f)) {
            spawn["x"] = sx;
            changed     = true;
        }
        if (ImGui::DragFloat("SpawnY", &sy, 1.0f)) {
            spawn["y"] = sy;
            changed     = true;
        }
    } else if (type == "set_scroll") {
        drawScroll(ensureObject(ev, "scroll"), changed);
    } else if (type == "set_background") {
        std::string bg = ev.value("backgroundId", "");
        if (comboString("Background", bg, assets.backgrounds)) {
            ev["backgroundId"] = bg;
            changed             = true;
        }
        if (inputText("##background", bg)) {
            ev["backgroundId"] = bg;
            changed             = true;
        }
    } else if (type == "set_music") {
        std::string music = ev.value("musicId", "");
        if (comboString("Music", music, assets.music)) {
            ev["musicId"] = music;
            changed        = true;
        }
        if (inputText("##music", music)) {
            ev["musicId"] = music;
            changed        = true;
        }
    } else if (type == "set_camera_bounds") {
        json& bounds = ensureObject(ev, "bounds");
        float minX   = static_cast<float>(bounds.value("minX", 0.0));
        float maxX   = static_cast<float>(bounds.value("maxX", 0.0));
        float minY   = static_cast<float>(bounds.value("minY", 0.0));
        float maxY   = static_cast<float>(bounds.value("maxY", 0.0));
        if (ImGui::DragFloat("MinX", &minX, 1.0f)) {
            bounds["minX"] = minX;
            changed         = true;
        }
        if (ImGui::DragFloat("MaxX", &maxX, 1.0f)) {
            bounds["maxX"] = maxX;
            changed         = true;
        }
        if (ImGui::DragFloat("MinY", &minY, 1.0f)) {
            bounds["minY"] = minY;
            changed         = true;
        }
        if (ImGui::DragFloat("MaxY", &maxY, 1.0f)) {
            bounds["maxY"] = maxY;
            changed         = true;
        }
    } else if (type == "set_player_bounds") {
        json& bounds = ensureObject(ev, "bounds");
        float minX   = static_cast<float>(bounds.value("minX", 0.0));
        float maxX   = static_cast<float>(bounds.value("maxX", 0.0));
        float minY   = static_cast<float>(bounds.value("minY", 0.0));
        float maxY   = static_cast<float>(bounds.value("maxY", 0.0));
        if (ImGui::DragFloat("MinX", &minX, 1.0f)) {
            bounds["minX"] = minX;
            changed         = true;
        }
        if (ImGui::DragFloat("MaxX", &maxX, 1.0f)) {
            bounds["maxX"] = maxX;
            changed         = true;
        }
        if (ImGui::DragFloat("MinY", &minY, 1.0f)) {
            bounds["minY"] = minY;
            changed         = true;
        }
        if (ImGui::DragFloat("MaxY", &maxY, 1.0f)) {
            bounds["maxY"] = maxY;
            changed         = true;
        }
    } else if (type == "clear_player_bounds") {
    } else if (type == "gate_open" || type == "gate_close") {
        std::string gate = ev.value("gateId", "");
        if (comboString("GateId", gate, ids.spawnIds)) {
            ev["gateId"] = gate;
            changed       = true;
        }
        if (inputText("##gate", gate)) {
            ev["gateId"] = gate;
            changed       = true;
        }
    } else if (type == "checkpoint") {
        std::string cp = ev.value("checkpointId", "");
        if (comboString("CheckpointId", cp, ids.checkpointIds)) {
            ev["checkpointId"] = cp;
            changed             = true;
        }
        if (inputText("##checkpointId", cp)) {
            ev["checkpointId"] = cp;
            changed             = true;
        }
        json& respawn = ensureObject(ev, "respawn");
        float rx      = static_cast<float>(respawn.value("x", 0.0));
        float ry      = static_cast<float>(respawn.value("y", 0.0));
        if (ImGui::DragFloat("RespawnX", &rx, 1.0f)) {
            respawn["x"] = rx;
            changed       = true;
        }
        if (ImGui::DragFloat("RespawnY", &ry, 1.0f)) {
            respawn["y"] = ry;
            changed       = true;
        }
    }
}

void drawEvents(json& events, const IdCache& ids, const AssetIndex& assets, bool& changed)
{
    if (!events.is_array())
        events = json::array();
    if (ImGui::Button("Add event")) {
        events.push_back(makeDefaultEvent("spawn_wave"));
        changed = true;
    }
    for (std::size_t i = 0; i < events.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        std::string header = "Event " + std::to_string(i);
        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            drawEvent(events[i], ids, assets, changed);
            if (ImGui::Button("Remove")) {
                events.erase(events.begin() + static_cast<long>(i));
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
        }
        ImGui::PopID();
    }
}
} 

AssetIndex loadAssetIndex(const std::string& assetsPath, const std::string& animationsPath)
{
    AssetIndex assets;

    std::ifstream assetsFile(assetsPath);
    if (assetsFile) {
        nlohmann::json doc = nlohmann::json::parse(assetsFile, nullptr, false);
        if (doc.is_object()) {
            if (doc.contains("textures") && doc["textures"].is_array()) {
                for (const auto& entry : doc["textures"]) {
                    if (!entry.is_object())
                        continue;
                    std::string id   = entry.value("id", "");
                    std::string type = entry.value("type", "");
                    if (id.empty())
                        continue;
                    if (type == "background")
                        assets.backgrounds.push_back(id);
                    if (type == "sprite")
                        assets.sprites.push_back(id);
                }
            }
            if (doc.contains("sounds") && doc["sounds"].is_array()) {
                for (const auto& entry : doc["sounds"]) {
                    if (!entry.is_object())
                        continue;
                    std::string id   = entry.value("id", "");
                    std::string type = entry.value("type", "");
                    if (id.empty())
                        continue;
                    if (type == "music")
                        assets.music.push_back(id);
                }
            }
        }
    }

    std::ifstream animFile(animationsPath);
    if (animFile) {
        nlohmann::json doc = nlohmann::json::parse(animFile, nullptr, false);
        if (doc.is_object()) {
            if (doc.contains("animations") && doc["animations"].is_array()) {
                for (const auto& entry : doc["animations"]) {
                    if (!entry.is_object())
                        continue;
                    std::string id = entry.value("id", "");
                    if (!id.empty())
                        assets.animations.push_back(id);
                }
            }
            if (doc.contains("labels") && doc["labels"].is_object()) {
                for (auto it = doc["labels"].begin(); it != doc["labels"].end(); ++it) {
                    const std::string spriteId = it.key();
                    const auto& labelsObj      = it.value();
                    if (!labelsObj.is_object())
                        continue;
                    for (auto lit = labelsObj.begin(); lit != labelsObj.end(); ++lit) {
                        assets.labels[spriteId].push_back(lit.key());
                    }
                }
            }
        }
    }

    sortUnique(assets.backgrounds);
    sortUnique(assets.music);
    sortUnique(assets.sprites);
    sortUnique(assets.animations);
    for (auto& [_, labels] : assets.labels) {
        sortUnique(labels);
    }

    return assets;
}

LevelEditor::LevelEditor(const AssetIndex& assets) : assets_(assets)
{
    fileStyle_ = detectFileStyle();
    createNewLevel();
}

void LevelEditor::ensureRoot()
{
    if (!level_.is_object())
        level_ = makeDefaultLevel();
    if (!level_.contains("schemaVersion"))
        level_["schemaVersion"] = 1;
    if (!level_.contains("levelId"))
        level_["levelId"] = 1;
    ensureObject(level_, "meta");
    ensureArray(level_, "archetypes");
    ensureArray(level_, "patterns");
    ensureObject(level_, "templates");
    ensureArray(level_, "segments");
    if (!level_["templates"].contains("hitboxes"))
        level_["templates"]["hitboxes"] = json::object();
    if (!level_["templates"].contains("colliders"))
        level_["templates"]["colliders"] = json::object();
    if (!level_["templates"].contains("enemies"))
        level_["templates"]["enemies"] = json::object();
    if (!level_["templates"].contains("obstacles"))
        level_["templates"]["obstacles"] = json::object();
}

void LevelEditor::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        status_ = "Impossible d'ouvrir " + path;
        return;
    }
    try {
        json doc = json::parse(file);
        level_   = doc;
        dirty_   = false;
        status_  = "Chargé: " + path;
    } catch (const std::exception& e) {
        status_ = std::string("Erreur JSON: ") + e.what();
    }
}

void LevelEditor::saveToFile(const std::string& path)
{
    std::filesystem::path out(path);
    if (out.has_parent_path()) {
        std::filesystem::create_directories(out.parent_path());
    }

    std::ofstream file(out);
    if (!file) {
        status_ = "Impossible d'écrire " + path;
        return;
    }
    file << level_.dump(2);
    dirty_  = false;
    status_ = "Sauvegardé: " + path;
}

void LevelEditor::validate()
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "rtype_level_editor_validate.json";
    std::ofstream file(tmp);
    if (!file) {
        validation_ = "Validation: échec d'écriture temporaire";
        return;
    }
    file << level_.dump(2);
    file.close();

    LevelData data;
    LevelLoadError error;
    if (LevelLoader::loadFromPath(tmp.string(), data, error)) {
        validation_ = "Validation: OK";
    } else {
        std::ostringstream oss;
        oss << "Validation: " << error.message;
        if (!error.path.empty())
            oss << " | " << error.path;
        if (!error.jsonPointer.empty())
            oss << " | " << error.jsonPointer;
        validation_ = oss.str();
    }
}

void LevelEditor::createNewLevel()
{
    level_   = makeBaseLevel(assets_);
    dirty_   = true;
    rawJson_.clear();
    rawDirty_ = false;
    updateFilePath();
    status_ = "Nouveau niveau";
}

LevelEditor::LevelFileStyle LevelEditor::detectFileStyle() const
{
    std::filesystem::path root(LevelLoader::levelsRoot());
    if (!std::filesystem::exists(root))
        return LevelFileStyle::Plain;
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file())
            continue;
        const std::string name = entry.path().filename().string();
        if (name.rfind("level_0", 0) == 0 && name.size() >= 11 && name.substr(name.size() - 5) == ".json") {
            return LevelFileStyle::Padded2;
        }
        if (name.rfind("level_", 0) == 0 && name.size() >= 10 && name.substr(name.size() - 5) == ".json") {
            return LevelFileStyle::Plain;
        }
    }
    return LevelFileStyle::Plain;
}

std::string LevelEditor::makeFilePath(int levelId) const
{
    std::ostringstream oss;
    if (fileStyle_ == LevelFileStyle::Padded2 && levelId >= 0 && levelId < 10) {
        oss << "level_0" << levelId << ".json";
    } else {
        oss << "level_" << levelId << ".json";
    }
    std::filesystem::path root(LevelLoader::levelsRoot());
    return (root / oss.str()).string();
}

void LevelEditor::updateFilePath()
{
    int levelId = level_.value("levelId", 1);
    filePath_   = makeFilePath(levelId);
}

IdCache LevelEditor::buildIdCache() const
{
    IdCache ids;
    if (!level_.is_object())
        return ids;

    if (level_.contains("patterns") && level_["patterns"].is_array()) {
        for (const auto& p : level_["patterns"]) {
            if (p.contains("id"))
                ids.patternIds.push_back(p["id"].get<std::string>());
        }
    }

    if (level_.contains("templates") && level_["templates"].is_object()) {
        const auto& templates = level_["templates"];
        if (templates.contains("hitboxes"))
            ids.hitboxIds = objectKeys(templates["hitboxes"]);
        if (templates.contains("colliders"))
            ids.colliderIds = objectKeys(templates["colliders"]);
        if (templates.contains("enemies"))
            ids.enemyTemplateIds = objectKeys(templates["enemies"]);
        if (templates.contains("obstacles"))
            ids.obstacleTemplateIds = objectKeys(templates["obstacles"]);
    }

    if (level_.contains("bosses") && level_["bosses"].is_object())
        ids.bossIds = objectKeys(level_["bosses"]);

    std::vector<const json*> eventLists;
    if (level_.contains("segments") && level_["segments"].is_array()) {
        for (const auto& seg : level_["segments"]) {
            if (seg.contains("events"))
                eventLists.push_back(&seg["events"]);
        }
    }
    if (level_.contains("bosses") && level_["bosses"].is_object()) {
        for (auto it = level_["bosses"].begin(); it != level_["bosses"].end(); ++it) {
            const auto& boss = it.value();
            if (boss.contains("phases") && boss["phases"].is_array()) {
                for (const auto& phase : boss["phases"]) {
                    if (phase.contains("events"))
                        eventLists.push_back(&phase["events"]);
                }
            }
            if (boss.contains("onDeath"))
                eventLists.push_back(&boss["onDeath"]);
        }
    }

    for (const auto* list : eventLists) {
        if (!list->is_array())
            continue;
        for (const auto& ev : *list) {
            if (!ev.is_object())
                continue;
            std::string type = ev.value("type", "");
            if (type == "spawn_wave") {
                std::string id = ev.value("id", "");
                if (!id.empty())
                    ids.spawnIds.push_back(id);
            } else if (type == "spawn_obstacle") {
                std::string spawnId = ev.value("spawnId", ev.value("id", ""));
                if (!spawnId.empty())
                    ids.spawnIds.push_back(spawnId);
            } else if (type == "spawn_boss") {
                std::string spawnId = ev.value("spawnId", ev.value("id", ""));
                if (!spawnId.empty())
                    ids.spawnIds.push_back(spawnId);
            } else if (type == "checkpoint") {
                std::string cp = ev.value("checkpointId", "");
                if (!cp.empty())
                    ids.checkpointIds.push_back(cp);
            }
        }
    }

    sortUnique(ids.patternIds);
    sortUnique(ids.hitboxIds);
    sortUnique(ids.colliderIds);
    sortUnique(ids.enemyTemplateIds);
    sortUnique(ids.obstacleTemplateIds);
    sortUnique(ids.bossIds);
    sortUnique(ids.spawnIds);
    sortUnique(ids.checkpointIds);

    return ids;
}

void LevelEditor::drawHeader()
{
    ImGui::SeparatorText("Fichier");
    ImGui::Text("Chemin: %s", filePath_.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Sauver"))
        saveToFile(filePath_);
    ImGui::SameLine();
    if (ImGui::Button("Nouveau"))
        createNewLevel();
    ImGui::SameLine();
    if (ImGui::Button("Valider"))
        validate();
    ImGui::SameLine();
    if (dirty_)
        ImGui::TextUnformatted("*");
}

void LevelEditor::drawMeta()
{
    json& meta = ensureObject(level_, "meta");
    std::string name = meta.value("name", "");
    if (inputText("Nom", name)) {
        meta["name"] = name;
        dirty_        = true;
    }

    std::string bg = meta.value("backgroundId", "");
    if (comboString("Background", bg, assets_.backgrounds)) {
        meta["backgroundId"] = bg;
        dirty_                 = true;
    }
    if (inputText("##background", bg)) {
        meta["backgroundId"] = bg;
        dirty_                 = true;
    }

    std::string music = meta.value("musicId", "");
    if (comboString("Music", music, assets_.music)) {
        meta["musicId"] = music;
        dirty_            = true;
    }
    if (inputText("##music", music)) {
        meta["musicId"] = music;
        dirty_            = true;
    }

    std::string author = meta.value("author", "");
    if (inputText("Auteur", author)) {
        meta["author"] = author;
        dirty_          = true;
    }

    std::string difficulty = meta.value("difficulty", "");
    if (inputText("Difficulté", difficulty)) {
        meta["difficulty"] = difficulty;
        dirty_              = true;
    }

    int levelId = level_.value("levelId", 1);
    if (ImGui::DragInt("LevelId", &levelId, 1.0f, 1, 9999)) {
        level_["levelId"] = levelId;
        updateFilePath();
        dirty_ = true;
    }
}

void LevelEditor::drawArchetypes()
{
    json& archetypes = ensureArray(level_, "archetypes");
    if (ImGui::Button("Ajouter archetype")) {
        archetypes.push_back(json::object({{"typeId", 0}, {"spriteId", ""}, {"animId", ""}, {"layer", 0}}));
        dirty_ = true;
    }
    ImGui::Separator();
    for (std::size_t i = 0; i < archetypes.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        auto& entry = archetypes[i];
        if (!entry.is_object())
            entry = json::object();
        int typeId = entry.value("typeId", 0);
        if (ImGui::DragInt("TypeId", &typeId, 1.0f, 0, 65535)) {
            entry["typeId"] = typeId;
            dirty_            = true;
        }

        std::string sprite = entry.value("spriteId", "");
        if (comboString("Sprite", sprite, assets_.sprites)) {
            entry["spriteId"] = sprite;
            dirty_              = true;
        }
        if (inputText("##sprite", sprite)) {
            entry["spriteId"] = sprite;
            dirty_              = true;
        }

        std::string anim = entry.value("animId", "");
        auto animOptions = mergeAnimationOptions(assets_, sprite);
        if (comboString("Anim", anim, animOptions)) {
            entry["animId"] = anim;
            dirty_            = true;
        }
        if (inputText("##anim", anim)) {
            entry["animId"] = anim;
            dirty_            = true;
        }

        int layer = entry.value("layer", 0);
        if (ImGui::DragInt("Layer", &layer, 1.0f)) {
            entry["layer"] = layer;
            dirty_           = true;
        }

        if (ImGui::Button("Supprimer")) {
            archetypes.erase(archetypes.begin() + static_cast<long>(i));
            dirty_ = true;
            ImGui::PopID();
            break;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
}

void LevelEditor::drawPatterns()
{
    json& patterns = ensureArray(level_, "patterns");
    if (ImGui::Button("Ajouter pattern")) {
        patterns.push_back(json::object({{"id", "pattern"}, {"type", "linear"}, {"speed", 100.0}}));
        dirty_ = true;
    }
    ImGui::Separator();
    for (std::size_t i = 0; i < patterns.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        auto& p = patterns[i];
        if (!p.is_object())
            p = json::object();
        std::string id = p.value("id", "");
        if (inputText("Id", id)) {
            p["id"] = id;
            dirty_   = true;
        }
        std::string type = p.value("type", "linear");
        const std::vector<std::string> types = {"linear", "zigzag", "sine"};
        if (comboString("Type", type, types)) {
            p["type"] = type;
            dirty_     = true;
        }
        float speed = static_cast<float>(p.value("speed", 0.0));
        if (ImGui::DragFloat("Speed", &speed, 1.0f)) {
            p["speed"] = speed;
            dirty_      = true;
        }
        if (type == "zigzag" || type == "sine") {
            float amp = static_cast<float>(p.value("amplitude", 0.0));
            float freq = static_cast<float>(p.value("frequency", 0.0));
            if (ImGui::DragFloat("Amplitude", &amp, 1.0f)) {
                p["amplitude"] = amp;
                dirty_          = true;
            }
            if (ImGui::DragFloat("Frequency", &freq, 0.1f)) {
                p["frequency"] = freq;
                dirty_          = true;
            }
        }
        if (type == "sine") {
            float phase = static_cast<float>(p.value("phase", 0.0));
            if (ImGui::DragFloat("Phase", &phase, 0.1f)) {
                p["phase"] = phase;
                dirty_      = true;
            }
        }
        if (ImGui::Button("Supprimer")) {
            patterns.erase(patterns.begin() + static_cast<long>(i));
            dirty_ = true;
            ImGui::PopID();
            break;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
}

void LevelEditor::drawTemplates()
{
    json& templates = ensureObject(level_, "templates");
    json& hitboxes  = ensureObject(templates, "hitboxes");
    json& colliders = ensureObject(templates, "colliders");
    json& enemies   = ensureObject(templates, "enemies");
    json& obstacles = ensureObject(templates, "obstacles");

    if (ImGui::BeginTabBar("Templates")) {
        if (ImGui::BeginTabItem("Hitboxes")) {
            if (ImGui::Button("Ajouter hitbox")) {
                std::string id = uniqueId("hitbox", objectKeys(hitboxes));
                hitboxes[id]   = makeDefaultHitbox();
                dirty_         = true;
            }
            std::vector<std::pair<std::string, std::string>> renames;
            for (const auto& key : objectKeys(hitboxes)) {
                ImGui::PushID(key.c_str());
                json& hb = hitboxes[key];
                std::string newKey = key;
                if (inputText("Id", newKey)) {
                    renames.emplace_back(key, newKey);
                }
                float width  = static_cast<float>(hb.value("width", 0.0));
                float height = static_cast<float>(hb.value("height", 0.0));
                float offsetX = static_cast<float>(hb.value("offsetX", 0.0));
                float offsetY = static_cast<float>(hb.value("offsetY", 0.0));
                bool active   = hb.value("active", true);
                if (ImGui::DragFloat("Width", &width, 1.0f)) {
                    hb["width"] = width;
                    dirty_       = true;
                }
                if (ImGui::DragFloat("Height", &height, 1.0f)) {
                    hb["height"] = height;
                    dirty_        = true;
                }
                if (ImGui::DragFloat("OffsetX", &offsetX, 1.0f)) {
                    hb["offsetX"] = offsetX;
                    dirty_         = true;
                }
                if (ImGui::DragFloat("OffsetY", &offsetY, 1.0f)) {
                    hb["offsetY"] = offsetY;
                    dirty_         = true;
                }
                if (ImGui::Checkbox("Active", &active)) {
                    hb["active"] = active;
                    dirty_        = true;
                }
                if (ImGui::Button("Supprimer")) {
                    hitboxes.erase(key);
                    dirty_ = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            for (const auto& [from, to] : renames) {
                if (renameKey(hitboxes, from, to))
                    dirty_ = true;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Colliders")) {
            if (ImGui::Button("Ajouter collider")) {
                std::string id = uniqueId("collider", objectKeys(colliders));
                colliders[id]  = makeDefaultCollider();
                dirty_         = true;
            }
            std::vector<std::pair<std::string, std::string>> renames;
            for (const auto& key : objectKeys(colliders)) {
                ImGui::PushID(key.c_str());
                json& col = colliders[key];
                std::string newKey = key;
                if (inputText("Id", newKey))
                    renames.emplace_back(key, newKey);

                std::string shape = col.value("shape", "box");
                const std::vector<std::string> shapes = {"box", "circle", "polygon"};
                if (comboString("Shape", shape, shapes)) {
                    col["shape"] = shape;
                    dirty_        = true;
                }

                float offsetX = static_cast<float>(col.value("offsetX", 0.0));
                float offsetY = static_cast<float>(col.value("offsetY", 0.0));
                bool active   = col.value("active", true);
                if (ImGui::DragFloat("OffsetX", &offsetX, 1.0f)) {
                    col["offsetX"] = offsetX;
                    dirty_          = true;
                }
                if (ImGui::DragFloat("OffsetY", &offsetY, 1.0f)) {
                    col["offsetY"] = offsetY;
                    dirty_          = true;
                }
                if (ImGui::Checkbox("Active", &active)) {
                    col["active"] = active;
                    dirty_         = true;
                }

                if (shape == "box") {
                    float width  = static_cast<float>(col.value("width", 0.0));
                    float height = static_cast<float>(col.value("height", 0.0));
                    if (ImGui::DragFloat("Width", &width, 1.0f)) {
                        col["width"] = width;
                        dirty_        = true;
                    }
                    if (ImGui::DragFloat("Height", &height, 1.0f)) {
                        col["height"] = height;
                        dirty_         = true;
                    }
                } else if (shape == "circle") {
                    float radius = static_cast<float>(col.value("radius", 0.0));
                    if (ImGui::DragFloat("Radius", &radius, 1.0f)) {
                        col["radius"] = radius;
                        dirty_         = true;
                    }
                } else if (shape == "polygon") {
                    json& points = ensureArray(col, "points");
                    if (ImGui::Button("Ajouter point")) {
                        points.push_back(json::array({0.0, 0.0}));
                        dirty_ = true;
                    }
                    for (std::size_t i = 0; i < points.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i));
                        if (!points[i].is_array() || points[i].size() != 2)
                            points[i] = json::array({0.0, 0.0});
                        float v[2] = {points[i][0].get<float>(), points[i][1].get<float>()};
                        if (ImGui::DragFloat2("Point", v, 1.0f)) {
                            points[i][0] = v[0];
                            points[i][1] = v[1];
                            dirty_        = true;
                        }
                        if (ImGui::Button("Supprimer")) {
                            points.erase(points.begin() + static_cast<long>(i));
                            dirty_ = true;
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopID();
                    }
                }

                if (ImGui::Button("Supprimer")) {
                    colliders.erase(key);
                    dirty_ = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            for (const auto& [from, to] : renames) {
                if (renameKey(colliders, from, to))
                    dirty_ = true;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Enemies")) {
            if (ImGui::Button("Ajouter enemy")) {
                std::string id = uniqueId("enemy", objectKeys(enemies));
                enemies[id]    = makeDefaultEnemy();
                dirty_         = true;
            }
            std::vector<std::pair<std::string, std::string>> renames;
            for (const auto& key : objectKeys(enemies)) {
                ImGui::PushID(key.c_str());
                json& enemy = enemies[key];
                std::string newKey = key;
                if (inputText("Id", newKey))
                    renames.emplace_back(key, newKey);

                int typeId = enemy.value("typeId", 0);
                if (ImGui::DragInt("TypeId", &typeId, 1.0f, 0, 65535)) {
                    enemy["typeId"] = typeId;
                    dirty_           = true;
                }

                std::string hb = enemy.value("hitbox", "");
                if (comboString("Hitbox", hb, objectKeys(hitboxes))) {
                    enemy["hitbox"] = hb;
                    dirty_           = true;
                }
                if (inputText("##hitbox", hb)) {
                    enemy["hitbox"] = hb;
                    dirty_           = true;
                }

                std::string col = enemy.value("collider", "");
                if (comboString("Collider", col, objectKeys(colliders))) {
                    enemy["collider"] = col;
                    dirty_             = true;
                }
                if (inputText("##collider", col)) {
                    enemy["collider"] = col;
                    dirty_             = true;
                }

                int health = enemy.value("health", 1);
                int score  = enemy.value("score", 0);
                if (ImGui::DragInt("Health", &health)) {
                    enemy["health"] = health;
                    dirty_           = true;
                }
                if (ImGui::DragInt("Score", &score)) {
                    enemy["score"] = score;
                    dirty_          = true;
                }

                if (drawVec2(enemy, "scale", "Scale", 1.0f, 1.0f))
                    dirty_ = true;

                bool hasShooting = enemy.contains("shooting");
                if (ImGui::Checkbox("Shooting", &hasShooting)) {
                    if (hasShooting)
                        enemy["shooting"] = json::object({{"interval", 1.5}, {"speed", 300.0}, {"damage", 5},
                                                           {"lifetime", 3.0}});
                    else
                        enemy.erase("shooting");
                    dirty_ = true;
                }
                if (enemy.contains("shooting")) {
                    json& shooting = enemy["shooting"];
                    float interval = static_cast<float>(shooting.value("interval", 1.5));
                    float speed    = static_cast<float>(shooting.value("speed", 300.0));
                    int damage     = shooting.value("damage", 5);
                    float lifetime = static_cast<float>(shooting.value("lifetime", 3.0));
                    if (ImGui::DragFloat("Interval", &interval, 0.05f)) {
                        shooting["interval"] = interval;
                        dirty_                = true;
                    }
                    if (ImGui::DragFloat("Speed", &speed, 1.0f)) {
                        shooting["speed"] = speed;
                        dirty_             = true;
                    }
                    if (ImGui::DragInt("Damage", &damage)) {
                        shooting["damage"] = damage;
                        dirty_              = true;
                    }
                    if (ImGui::DragFloat("Lifetime", &lifetime, 0.05f)) {
                        shooting["lifetime"] = lifetime;
                        dirty_                = true;
                    }
                }

                if (ImGui::Button("Supprimer")) {
                    enemies.erase(key);
                    dirty_ = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            for (const auto& [from, to] : renames) {
                if (renameKey(enemies, from, to))
                    dirty_ = true;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Obstacles")) {
            if (ImGui::Button("Ajouter obstacle")) {
                std::string id = uniqueId("obstacle", objectKeys(obstacles));
                obstacles[id]  = makeDefaultObstacle();
                dirty_         = true;
            }
            std::vector<std::pair<std::string, std::string>> renames;
            for (const auto& key : objectKeys(obstacles)) {
                ImGui::PushID(key.c_str());
                json& ob = obstacles[key];
                std::string newKey = key;
                if (inputText("Id", newKey))
                    renames.emplace_back(key, newKey);

                int typeId = ob.value("typeId", 0);
                if (ImGui::DragInt("TypeId", &typeId, 1.0f, 0, 65535)) {
                    ob["typeId"] = typeId;
                    dirty_        = true;
                }

                std::string hb = ob.value("hitbox", "");
                if (comboString("Hitbox", hb, objectKeys(hitboxes))) {
                    ob["hitbox"] = hb;
                    dirty_        = true;
                }
                if (inputText("##hitbox", hb)) {
                    ob["hitbox"] = hb;
                    dirty_        = true;
                }

                std::string col = ob.value("collider", "");
                if (comboString("Collider", col, objectKeys(colliders))) {
                    ob["collider"] = col;
                    dirty_          = true;
                }
                if (inputText("##collider", col)) {
                    ob["collider"] = col;
                    dirty_          = true;
                }

                int health = ob.value("health", 1);
                if (ImGui::DragInt("Health", &health)) {
                    ob["health"] = health;
                    dirty_        = true;
                }

                std::string anchor = ob.value("anchor", "absolute");
                const std::vector<std::string> anchors = {"absolute", "top", "bottom"};
                if (comboString("Anchor", anchor, anchors)) {
                    ob["anchor"] = anchor;
                    dirty_        = true;
                }

                float margin = static_cast<float>(ob.value("margin", 0.0));
                float speedX = static_cast<float>(ob.value("speedX", 0.0));
                float speedY = static_cast<float>(ob.value("speedY", 0.0));
                if (ImGui::DragFloat("Margin", &margin, 1.0f)) {
                    ob["margin"] = margin;
                    dirty_        = true;
                }
                if (ImGui::DragFloat("SpeedX", &speedX, 1.0f)) {
                    ob["speedX"] = speedX;
                    dirty_        = true;
                }
                if (ImGui::DragFloat("SpeedY", &speedY, 1.0f)) {
                    ob["speedY"] = speedY;
                    dirty_        = true;
                }

                if (drawVec2(ob, "scale", "Scale", 1.0f, 1.0f))
                    dirty_ = true;

                if (ImGui::Button("Supprimer")) {
                    obstacles.erase(key);
                    dirty_ = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            for (const auto& [from, to] : renames) {
                if (renameKey(obstacles, from, to))
                    dirty_ = true;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void LevelEditor::drawBosses()
{
    if (!level_.contains("bosses") || !level_["bosses"].is_object())
        level_["bosses"] = json::object();
    json& bosses = level_["bosses"];

    if (ImGui::Button("Ajouter boss")) {
        std::string id = uniqueId("boss", objectKeys(bosses));
        bosses[id]     = makeDefaultBoss();
        dirty_         = true;
    }

    std::vector<std::pair<std::string, std::string>> renames;
    for (const auto& key : objectKeys(bosses)) {
        ImGui::PushID(key.c_str());
        json& boss = bosses[key];
        std::string newKey = key;
        if (inputText("Id", newKey))
            renames.emplace_back(key, newKey);

        int typeId = boss.value("typeId", 0);
        if (ImGui::DragInt("TypeId", &typeId, 1.0f, 0, 65535)) {
            boss["typeId"] = typeId;
            dirty_          = true;
        }

        std::string hitbox = boss.value("hitbox", "");
        if (comboString("Hitbox", hitbox, idCache_.hitboxIds)) {
            boss["hitbox"] = hitbox;
            dirty_          = true;
        }
        if (inputText("##hitbox", hitbox)) {
            boss["hitbox"] = hitbox;
            dirty_          = true;
        }

        std::string collider = boss.value("collider", "");
        if (comboString("Collider", collider, idCache_.colliderIds)) {
            boss["collider"] = collider;
            dirty_            = true;
        }
        if (inputText("##collider", collider)) {
            boss["collider"] = collider;
            dirty_            = true;
        }

        int health = boss.value("health", 1);
        int score  = boss.value("score", 0);
        if (ImGui::DragInt("Health", &health)) {
            boss["health"] = health;
            dirty_          = true;
        }
        if (ImGui::DragInt("Score", &score)) {
            boss["score"] = score;
            dirty_         = true;
        }

        if (drawVec2(boss, "scale", "Scale", 1.0f, 1.0f))
            dirty_ = true;

        bool hasPattern = boss.contains("patternId");
        if (ImGui::Checkbox("Pattern", &hasPattern)) {
            if (hasPattern)
                boss["patternId"] = "";
            else
                boss.erase("patternId");
            dirty_ = true;
        }
        if (boss.contains("patternId")) {
            std::string pattern = boss.value("patternId", "");
            if (comboString("##pattern", pattern, idCache_.patternIds)) {
                boss["patternId"] = pattern;
                dirty_             = true;
            }
            if (inputText("##patternText", pattern)) {
                boss["patternId"] = pattern;
                dirty_             = true;
            }
        }

        bool hasShooting = boss.contains("shooting");
        if (ImGui::Checkbox("Shooting", &hasShooting)) {
            if (hasShooting)
                boss["shooting"] = json::object({{"interval", 1.5}, {"speed", 300.0}, {"damage", 5},
                                                  {"lifetime", 3.0}});
            else
                boss.erase("shooting");
            dirty_ = true;
        }
        if (boss.contains("shooting")) {
            json& shooting = boss["shooting"];
            float interval = static_cast<float>(shooting.value("interval", 1.5));
            float speed    = static_cast<float>(shooting.value("speed", 300.0));
            int damage     = shooting.value("damage", 5);
            float lifetime = static_cast<float>(shooting.value("lifetime", 3.0));
            if (ImGui::DragFloat("Interval", &interval, 0.05f)) {
                shooting["interval"] = interval;
                dirty_                = true;
            }
            if (ImGui::DragFloat("Speed", &speed, 1.0f)) {
                shooting["speed"] = speed;
                dirty_             = true;
            }
            if (ImGui::DragInt("Damage", &damage)) {
                shooting["damage"] = damage;
                dirty_              = true;
            }
            if (ImGui::DragFloat("Lifetime", &lifetime, 0.05f)) {
                shooting["lifetime"] = lifetime;
                dirty_                = true;
            }
        }

        json& phases = ensureArray(boss, "phases");
        if (ImGui::CollapsingHeader("Phases")) {
            if (ImGui::Button("Ajouter phase")) {
                phases.push_back(json::object({{"id", "phase"}, {"trigger", makeDefaultTrigger("time")},
                                               {"events", json::array()}}));
                dirty_ = true;
            }
            for (std::size_t i = 0; i < phases.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                json& phase = phases[i];
                std::string pid = phase.value("id", "");
                if (inputText("PhaseId", pid)) {
                    phase["id"] = pid;
                    dirty_       = true;
                }
                if (!phase.contains("trigger"))
                    phase["trigger"] = makeDefaultTrigger("time");
                if (ImGui::CollapsingHeader("Trigger")) {
                    drawTrigger(phase["trigger"], idCache_, dirty_);
                }
                if (!phase.contains("events"))
                    phase["events"] = json::array();
                if (ImGui::CollapsingHeader("Events")) {
                    drawEvents(phase["events"], idCache_, assets_, dirty_);
                }
                if (ImGui::Button("Supprimer phase")) {
                    phases.erase(phases.begin() + static_cast<long>(i));
                    dirty_ = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }

        json& onDeath = ensureArray(boss, "onDeath");
        if (ImGui::CollapsingHeader("OnDeath")) {
            drawEvents(onDeath, idCache_, assets_, dirty_);
        }

        if (ImGui::Button("Supprimer")) {
            bosses.erase(key);
            dirty_ = true;
            ImGui::PopID();
            break;
        }
        ImGui::Separator();
        ImGui::PopID();
    }

    for (const auto& [from, to] : renames) {
        if (renameKey(bosses, from, to))
            dirty_ = true;
    }
}

void LevelEditor::drawSegments()
{
    json& segments = ensureArray(level_, "segments");
    if (ImGui::Button("Ajouter segment")) {
        segments.push_back(makeDefaultSegment());
        dirty_ = true;
    }

    for (std::size_t i = 0; i < segments.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        json& seg = segments[i];
        if (!seg.is_object())
            seg = makeDefaultSegment();

        std::string header = "Segment " + std::to_string(i);
        if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            std::string id = seg.value("id", "");
            if (inputText("Id", id)) {
                seg["id"] = id;
                dirty_     = true;
            }

            if (ImGui::CollapsingHeader("Scroll")) {
                drawScroll(ensureObject(seg, "scroll"), dirty_);
            }

            bool bossRoom = seg.value("bossRoom", false);
            if (ImGui::Checkbox("BossRoom", &bossRoom)) {
                seg["bossRoom"] = bossRoom;
                dirty_           = true;
            }

            bool hasBounds = seg.contains("cameraBounds");
            if (ImGui::Checkbox("CameraBounds", &hasBounds)) {
                if (hasBounds)
                    seg["cameraBounds"] = json::object({{"minX", 0.0}, {"maxX", 0.0}, {"minY", 0.0}, {"maxY", 0.0}});
                else
                    seg.erase("cameraBounds");
                dirty_ = true;
            }
            if (seg.contains("cameraBounds")) {
                json& bounds = seg["cameraBounds"];
                float minX   = static_cast<float>(bounds.value("minX", 0.0));
                float maxX   = static_cast<float>(bounds.value("maxX", 0.0));
                float minY   = static_cast<float>(bounds.value("minY", 0.0));
                float maxY   = static_cast<float>(bounds.value("maxY", 0.0));
                if (ImGui::DragFloat("MinX", &minX, 1.0f)) {
                    bounds["minX"] = minX;
                    dirty_          = true;
                }
                if (ImGui::DragFloat("MaxX", &maxX, 1.0f)) {
                    bounds["maxX"] = maxX;
                    dirty_          = true;
                }
                if (ImGui::DragFloat("MinY", &minY, 1.0f)) {
                    bounds["minY"] = minY;
                    dirty_          = true;
                }
                if (ImGui::DragFloat("MaxY", &maxY, 1.0f)) {
                    bounds["maxY"] = maxY;
                    dirty_          = true;
                }
            }

            if (ImGui::CollapsingHeader("Events")) {
                drawEvents(ensureArray(seg, "events"), idCache_, assets_, dirty_);
            }

            if (!seg.contains("exit"))
                seg["exit"] = makeDefaultTrigger("distance");
            if (ImGui::CollapsingHeader("Exit")) {
                drawTrigger(seg["exit"], idCache_, dirty_);
            }

            if (ImGui::Button("Monter") && i > 0) {
                std::swap(segments[i], segments[i - 1]);
                dirty_ = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Descendre") && i + 1 < segments.size()) {
                std::swap(segments[i], segments[i + 1]);
                dirty_ = true;
            }

            if (ImGui::Button("Supprimer")) {
                segments.erase(segments.begin() + static_cast<long>(i));
                dirty_ = true;
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
        }
        ImGui::PopID();
    }
}

void LevelEditor::drawRawJson()
{
    if (rawJson_.empty())
        rawJson_ = level_.dump(2);
    if (inputTextMultiline("##raw", rawJson_, ImVec2(-1.0f, 300.0f))) {
        rawDirty_ = true;
    }
    if (ImGui::Button("Recharger depuis le niveau")) {
        rawJson_  = level_.dump(2);
        rawDirty_ = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Appliquer")) {
        try {
            json doc = json::parse(rawJson_);
            level_   = doc;
            updateFilePath();
            dirty_   = true;
            rawDirty_ = false;
            status_  = "JSON brut appliqué";
        } catch (const std::exception& e) {
            status_ = std::string("Erreur JSON brut: ") + e.what();
        }
    }
}

void LevelEditor::drawTabs()
{
    if (ImGui::BeginTabBar("LevelTabs")) {
        if (ImGui::BeginTabItem("Meta")) {
            drawMeta();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Archetypes")) {
            drawArchetypes();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Patterns")) {
            drawPatterns();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Templates")) {
            drawTemplates();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Bosses")) {
            drawBosses();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Segments")) {
            drawSegments();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("JSON brut")) {
            drawRawJson();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void LevelEditor::drawStatus()
{
    if (!status_.empty()) {
        ImGui::SeparatorText("Status");
        ImGui::TextWrapped("%s", status_.c_str());
    }
    if (!validation_.empty()) {
        ImGui::SeparatorText("Validation");
        ImGui::TextWrapped("%s", validation_.c_str());
    }
}

void LevelEditor::draw()
{
    ensureRoot();
    idCache_ = buildIdCache();

    ImGui::Begin("Level Editor");
    drawHeader();
    drawTabs();
    drawStatus();
    ImGui::End();
}
