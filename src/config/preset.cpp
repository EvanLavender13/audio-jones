#include "preset.h"
#include "app_configs.h"
#include "automation/drawable_params.h"
#include "config/effect_serialization.h"
#include "render/drawable.h"
#include "ui/imgui_panels.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AudioConfig, channelMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrawableBase, enabled, x, y,
                                                rotationSpeed, rotationAngle,
                                                opacity, drawInterval, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveformData, amplitudeScale,
                                                thickness, smoothness, radius,
                                                waveformMotionScale, colorShift,
                                                colorShiftSpeed, style,
                                                pointCount)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectrumData, innerRadius,
                                                barHeight, barWidth, smoothing,
                                                minDb, maxDb, colorShift,
                                                colorShiftSpeed)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShapeData, sides, width, height,
                                                aspectLocked, textured, texZoom,
                                                texAngle, texBrightness,
                                                texMotionScale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DualLissajousConfig, amplitude,
                                                motionSpeed, freqX1, freqY1,
                                                freqX2, freqY2, offsetX2,
                                                offsetY2)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RandomWalkConfig,
                                                RANDOM_WALK_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParametricTrailData,
                                                PARAMETRIC_TRAIL_DATA_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig, LFO_CONFIG_FIELDS)

static void to_json(json &j, const Drawable &d) {
  j["id"] = d.id;
  j["type"] = d.type;
  j["path"] = d.path;
  j["base"] = d.base;
  switch (d.type) {
  case DRAWABLE_WAVEFORM:
    j["waveform"] = d.waveform;
    break;
  case DRAWABLE_SPECTRUM:
    j["spectrum"] = d.spectrum;
    break;
  case DRAWABLE_SHAPE:
    j["shape"] = d.shape;
    break;
  case DRAWABLE_PARAMETRIC_TRAIL:
    j["parametricTrail"] = d.parametricTrail;
    break;
  }
}

static void from_json(const json &j, Drawable &d) {
  d = Drawable{};
  d.id = j.value("id", (uint32_t)0);
  d.type = j.value("type", DRAWABLE_WAVEFORM);
  d.path = j.value("path", PATH_CIRCULAR);
  d.base = j.value("base", DrawableBase{});
  switch (d.type) {
  case DRAWABLE_WAVEFORM:
    if (j.contains("waveform")) {
      d.waveform = j["waveform"].get<WaveformData>();
    }
    break;
  case DRAWABLE_SPECTRUM:
    if (j.contains("spectrum")) {
      d.spectrum = j["spectrum"].get<SpectrumData>();
    }
    break;
  case DRAWABLE_SHAPE:
    if (j.contains("shape")) {
      d.shape = j["shape"].get<ShapeData>();
    }
    break;
  case DRAWABLE_PARAMETRIC_TRAIL:
    if (j.contains("parametricTrail")) {
      d.parametricTrail = j["parametricTrail"].get<ParametricTrailData>();
    }
    break;
  }
}

void to_json(json &j, const Preset &p) {
  j["name"] = std::string(p.name);
  j["effects"] = p.effects;
  j["audio"] = p.audio;
  j["drawableCount"] = p.drawableCount;
  j["drawables"] = json::array();
  for (int i = 0; i < p.drawableCount; i++) {
    j["drawables"].push_back(p.drawables[i]);
  }
  j["modulation"] = p.modulation;
  j["lfos"] = json::array();
  for (int i = 0; i < NUM_LFOS; i++) {
    j["lfos"].push_back(p.lfos[i]);
  }
}

void from_json(const json &j, Preset &p) {
  const std::string name = j.at("name").get<std::string>();
  strncpy(p.name, name.c_str(), PRESET_NAME_MAX - 1);
  p.name[PRESET_NAME_MAX - 1] = '\0';
  p.effects = j.value("effects", EffectConfig{});
  p.audio = j.value("audio", AudioConfig{});
  p.drawableCount = j.value("drawableCount", 0);
  if (p.drawableCount > MAX_DRAWABLES) {
    p.drawableCount = MAX_DRAWABLES;
  }
  if (j.contains("drawables")) {
    const auto &arr = j["drawables"];
    for (int i = 0; i < MAX_DRAWABLES && i < (int)arr.size(); i++) {
      p.drawables[i] = arr[i].get<Drawable>();
    }
  }
  p.modulation = j.value("modulation", ModulationConfig{});
  if (j.contains("lfos")) {
    const auto &lfoArr = j["lfos"];
    for (int i = 0; i < NUM_LFOS && i < (int)lfoArr.size(); i++) {
      p.lfos[i] = lfoArr[i].get<LFOConfig>();
    }
  }
}

bool PresetSave(const Preset *preset, const char *filepath) {
  try {
    const json j = *preset;
    std::ofstream file(filepath);
    if (!file.is_open()) {
      return false;
    }
    file << j.dump(2);
    return true;
  } catch (...) {
    return false;
  }
}

bool PresetLoad(Preset *preset, const char *filepath) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      return false;
    }
    const json j = json::parse(file);
    *preset = j.get<Preset>();
    return true;
  } catch (...) {
    return false;
  }
}

int PresetListEntries(const char *directory, PresetEntry *entries,
                      int maxEntries) {
  try {
    if (!fs::exists(directory)) {
      fs::create_directories(directory);
      return 0;
    }

    static PresetEntry folders[MAX_PRESET_ENTRIES];
    static PresetEntry presets[MAX_PRESET_ENTRIES];
    int folderCount = 0;
    int presetCount = 0;

    for (const auto &entry : fs::directory_iterator(directory)) {
      if (entry.is_directory()) {
        if (folderCount >= MAX_PRESET_ENTRIES) {
          continue;
        }
        const std::string name = entry.path().filename().string();
        strncpy(folders[folderCount].name, name.c_str(), PRESET_PATH_MAX - 1);
        folders[folderCount].name[PRESET_PATH_MAX - 1] = '\0';
        folders[folderCount].isFolder = true;
        folderCount++;
      } else if (entry.path().extension() == ".json") {
        if (presetCount >= MAX_PRESET_ENTRIES) {
          continue;
        }
        const std::string name = entry.path().stem().string();
        strncpy(presets[presetCount].name, name.c_str(), PRESET_PATH_MAX - 1);
        presets[presetCount].name[PRESET_PATH_MAX - 1] = '\0';
        presets[presetCount].isFolder = false;
        presetCount++;
      }
    }

    auto cmpCaseInsensitive = [](const PresetEntry &a, const PresetEntry &b) {
#ifdef _MSC_VER
      return _stricmp(a.name, b.name) < 0;
#else
      return strcasecmp(a.name, b.name) < 0;
#endif
    };

    std::sort(folders, folders + folderCount, cmpCaseInsensitive);
    std::sort(presets, presets + presetCount, cmpCaseInsensitive);

    int count = 0;
    for (int i = 0; i < folderCount && count < maxEntries; i++) {
      entries[count++] = folders[i];
    }
    for (int i = 0; i < presetCount && count < maxEntries; i++) {
      entries[count++] = presets[i];
    }
    return count;
  } catch (...) {
    return 0;
  }
}

void PresetFromAppConfigs(Preset *preset, const AppConfigs *configs) {
  // Write base values before copying (next ModEngineUpdate restores modulation)
  ModEngineWriteBaseValues();

  preset->effects = *configs->effects;
  preset->audio = *configs->audio;
  preset->drawableCount = *configs->drawableCount;
  for (int i = 0; i < *configs->drawableCount; i++) {
    preset->drawables[i] = configs->drawables[i];
  }
  ModulationConfigFromEngine(&preset->modulation);
  ModulationConfigStripDisabledRoutes(&preset->modulation, configs->effects);
  for (int i = 0; i < NUM_LFOS; i++) {
    preset->lfos[i] = configs->lfos[i];
  }
}

void PresetToAppConfigs(const Preset *preset, AppConfigs *configs) {
  *configs->effects = preset->effects;
  *configs->audio = preset->audio;
  // Clear old drawable params before loading new preset to avoid stale pointers
  for (uint32_t i = 1; i <= MAX_DRAWABLES; i++) {
    DrawableParamsUnregister(i);
  }
  *configs->drawableCount = preset->drawableCount;
  for (int i = 0; i < preset->drawableCount; i++) {
    configs->drawables[i] = preset->drawables[i];
  }
  ImGuiDrawDrawablesSyncIdCounter(configs->drawables, *configs->drawableCount);
  DrawableParamsSyncAll(configs->drawables, *configs->drawableCount);
  // Load LFO configs before ModulationConfigToEngine so SyncBases captures
  // correct rates
  for (int i = 0; i < NUM_LFOS; i++) {
    configs->lfos[i] = preset->lfos[i];
  }
  ModulationConfigToEngine(&preset->modulation);
}
