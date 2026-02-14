#include "preset.h"
#include "app_configs.h"
#include "automation/drawable_params.h"
#include "config/effect_serialization.h"
#include "render/drawable.h"
#include "ui/imgui_panels.h"
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
                                                colorShiftSpeed)
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParametricTrailData, lissajous,
                                                shapeType, size, filled,
                                                strokeThickness, gateFreq)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig, enabled, rate,
                                                waveform)

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

Preset PresetDefault(void) {
  Preset p = {};
  strncpy(p.name, "Default", PRESET_NAME_MAX);
  p.effects = EffectConfig{};
  p.audio = AudioConfig{};
  p.drawableCount = 0;
  for (int i = 0; i < NUM_LFOS; i++) {
    p.lfos[i] = LFOConfig{};
  }
  return p;
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

int PresetListFiles(const char *directory, char outFiles[][PRESET_PATH_MAX],
                    int maxFiles) {
  int count = 0;
  try {
    if (!fs::exists(directory)) {
      fs::create_directories(directory);
      return 0;
    }
    for (const auto &entry : fs::directory_iterator(directory)) {
      if (count >= maxFiles) {
        break;
      }
      if (entry.path().extension() == ".json") {
        const std::string filename = entry.path().filename().string();
        strncpy(outFiles[count], filename.c_str(), PRESET_PATH_MAX - 1);
        outFiles[count][PRESET_PATH_MAX - 1] = '\0';
        count++;
      }
    }
  } catch (...) {
    return count;
  }
  return count;
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
