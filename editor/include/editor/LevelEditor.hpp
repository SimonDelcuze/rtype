#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

struct AssetIndex
{
    std::vector<std::string> backgrounds;
    std::vector<std::string> music;
    std::vector<std::string> sprites;
    std::vector<std::string> animations;
    std::unordered_map<std::string, std::vector<std::string>> labels;
};

struct IdCache
{
    std::vector<std::string> patternIds;
    std::vector<std::string> hitboxIds;
    std::vector<std::string> colliderIds;
    std::vector<std::string> enemyTemplateIds;
    std::vector<std::string> obstacleTemplateIds;
    std::vector<std::string> bossIds;
    std::vector<std::string> spawnIds;
    std::vector<std::string> checkpointIds;
};

AssetIndex loadAssetIndex(const std::string& assetsPath, const std::string& animationsPath);

class LevelEditor
{
  public:
    explicit LevelEditor(const AssetIndex& assets);
    void draw();

  private:
    using json = nlohmann::ordered_json;

    AssetIndex assets_;
    json level_;
    std::string filePath_;
    bool dirty_ = false;
    std::string status_;
    std::string validation_;
    std::string rawJson_;
    bool rawDirty_ = false;
    IdCache idCache_;

    enum class LevelFileStyle { Plain, Padded2 };
    LevelFileStyle fileStyle_ = LevelFileStyle::Plain;

    void ensureRoot();
    void loadFromFile(const std::string& path);
    void saveToFile(const std::string& path);
    void validate();
    void createNewLevel();
    void updateFilePath();
    LevelFileStyle detectFileStyle() const;
    std::string makeFilePath(std::int32_t levelId) const;

    void drawHeader();
    void drawTabs();
    void drawMeta();
    void drawArchetypes();
    void drawPatterns();
    void drawTemplates();
    void drawBosses();
    void drawSegments();
    void drawRawJson();
    void drawStatus();

    IdCache buildIdCache() const;
};
