#include "preset.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>

using json = nlohmann::json;
namespace fs = std::filesystem;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaveformConfig,
    amplitudeScale, thickness, hueOffset, smoothness, radius)

void to_json(json& j, const Preset& p) {
    j["name"] = std::string(p.name);
    j["halfLife"] = p.halfLife;
    j["waveformCount"] = p.waveformCount;
    j["waveforms"] = json::array();
    for (int i = 0; i < MAX_WAVEFORMS; i++) {
        j["waveforms"].push_back(p.waveforms[i]);
    }
}

void from_json(const json& j, Preset& p) {
    std::string name = j.at("name").get<std::string>();
    strncpy(p.name, name.c_str(), PRESET_NAME_MAX - 1);
    p.name[PRESET_NAME_MAX - 1] = '\0';
    p.halfLife = j.at("halfLife").get<float>();
    p.waveformCount = j.at("waveformCount").get<int>();
    auto& arr = j.at("waveforms");
    for (int i = 0; i < MAX_WAVEFORMS && i < (int)arr.size(); i++) {
        p.waveforms[i] = arr[i].get<WaveformConfig>();
    }
}

extern "C" {

Preset PresetDefault(void) {
    Preset p = {};
    strncpy(p.name, "Default", PRESET_NAME_MAX);
    p.halfLife = 0.5f;
    p.waveformCount = 1;
    p.waveforms[0] = WaveformConfigDefault();
    return p;
}

bool PresetSave(const Preset* preset, const char* filepath) {
    try {
        json j = *preset;
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

bool PresetLoad(Preset* preset, const char* filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;
        json j = json::parse(file);
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
            if (count >= maxFiles) break;
            if (entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
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

}
