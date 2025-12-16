#include "preset.h"
#include "app_configs.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>

using json = nlohmann::json;
namespace fs = std::filesystem;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorConfig,
    mode, solid, rainbowHue, rainbowRange, rainbowSat, rainbowVal)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LFOConfig,
    enabled, rate, waveform)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysarumConfig,
    enabled, agentCount, sensorDistance, sensorAngle, turningAngle,
    stepSize, depositAmount, decayHalfLife, diffusionScale, boostIntensity,
    debugOverlay, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EffectConfig,
    halfLife, baseBlurScale, beatBlurScale, chromaticMaxOffset,
    feedbackZoom, feedbackRotation, feedbackDesaturate,
    kaleidoSegments, voronoiScale, voronoiIntensity, voronoiSpeed,
    voronoiEdgeWidth, rotationLFO, physarum)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AudioConfig, channelMode)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveformConfig,
    amplitudeScale, thickness, smoothness, radius, rotationSpeed, rotationOffset, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectrumConfig,
    enabled, innerRadius, barHeight, barWidth, smoothing,
    minDb, maxDb, rotationSpeed, rotationOffset, color)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BandConfig,
    bassSensitivity, midSensitivity, trebSensitivity)

void to_json(json& j, const Preset& p) {
    j["name"] = std::string(p.name);
    j["effects"] = p.effects;
    j["audio"] = p.audio;
    j["waveformCount"] = p.waveformCount;
    j["waveforms"] = json::array();
    for (int i = 0; i < p.waveformCount; i++) {
        j["waveforms"].push_back(p.waveforms[i]);
    }
    j["spectrum"] = p.spectrum;
    j["bands"] = p.bands;
}

void from_json(const json& j, Preset& p) {
    const std::string name = j.at("name").get<std::string>();
    strncpy(p.name, name.c_str(), PRESET_NAME_MAX - 1);
    p.name[PRESET_NAME_MAX - 1] = '\0';
    p.effects = j.value("effects", EffectConfig{});
    p.audio = j.value("audio", AudioConfig{});
    p.waveformCount = j.at("waveformCount").get<int>();
    if (p.waveformCount < 1) {
        p.waveformCount = 1;
    }
    if (p.waveformCount > MAX_WAVEFORMS) {
        p.waveformCount = MAX_WAVEFORMS;
    }
    const auto& arr = j.at("waveforms");
    for (int i = 0; i < MAX_WAVEFORMS && i < (int)arr.size(); i++) {
        p.waveforms[i] = arr[i].get<WaveformConfig>();
    }
    p.spectrum = j.value("spectrum", SpectrumConfig{});
    p.bands = j.value("bands", BandConfig{});
}

Preset PresetDefault(void) {
    Preset p = {};
    strncpy(p.name, "Default", PRESET_NAME_MAX);
    p.effects = EffectConfig{};
    p.audio = AudioConfig{};
    p.waveformCount = 1;
    p.waveforms[0] = WaveformConfig{};
    p.spectrum = SpectrumConfig{};
    p.bands = BandConfig{};
    return p;
}

bool PresetSave(const Preset* preset, const char* filepath) {
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

bool PresetLoad(Preset* preset, const char* filepath) {
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

int PresetListFiles(const char* directory, char outFiles[][PRESET_PATH_MAX], int maxFiles) {
    int count = 0;
    try {
        if (!fs::exists(directory)) {
            fs::create_directories(directory);
            return 0;
        }
        for (const auto& entry : fs::directory_iterator(directory)) {
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

void PresetFromAppConfigs(Preset* preset, const AppConfigs* configs) {
    preset->effects = *configs->effects;
    preset->audio = *configs->audio;
    preset->waveformCount = *configs->waveformCount;
    for (int i = 0; i < *configs->waveformCount; i++) {
        preset->waveforms[i] = configs->waveforms[i];
    }
    preset->spectrum = *configs->spectrum;
    preset->bands = *configs->bands;
}

void PresetToAppConfigs(const Preset* preset, AppConfigs* configs) {
    *configs->effects = preset->effects;
    *configs->audio = preset->audio;
    *configs->waveformCount = preset->waveformCount;
    for (int i = 0; i < preset->waveformCount; i++) {
        configs->waveforms[i] = preset->waveforms[i];
    }
    *configs->spectrum = preset->spectrum;
    *configs->bands = preset->bands;
}
