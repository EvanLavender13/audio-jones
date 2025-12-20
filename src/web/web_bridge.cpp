#include "config/preset.h"
#include "web_bridge.h"

#include <stdio.h>
#include <string.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "config/app_configs.h"
#include "analysis/beat.h"
#include "analysis/bands.h"

using json = nlohmann::json;

// Validate preset filename to prevent path traversal attacks.
// Rejects filenames containing path separators or parent directory references.
static bool IsValidPresetFilename(const char* filename)
{
    if (filename == NULL || filename[0] == '\0') {
        return false;
    }

    // Reject path separators and parent directory references
    if (strstr(filename, "..") != NULL ||
        strchr(filename, '/') != NULL ||
        strchr(filename, '\\') != NULL) {
        return false;
    }

    return true;
}

// Build full path to preset file in presets/ directory
static void BuildPresetPath(char* outPath, int maxLen, const char* filename)
{
    snprintf(outPath, maxLen, "presets/%s", filename);
}

static void SerializeJsonToBuffer(const json& msg, char* outJson, int maxLen)
{
    if (outJson == NULL || maxLen <= 0) {
        return;
    }
    std::string str = msg.dump();
    snprintf(outJson, maxLen, "%s", str.c_str());
}

void WebBridgeSerializeAnalysis(const BeatDetector* beat,
                                 const BandEnergies* bands,
                                 char* outJson, int maxLen)
{
    if (beat == NULL || bands == NULL) {
        SerializeJsonToBuffer(json({}), outJson, maxLen);
        return;
    }

    const float beatIntensity = BeatDetectorGetIntensity(beat);

    // Minimum threshold to avoid near-zero division producing Infinity
    const float MIN_AVG = 0.001f;
    const float bassNorm = (bands->bassAvg > MIN_AVG) ? bands->bassSmooth / bands->bassAvg : 0.0f;
    const float midNorm = (bands->midAvg > MIN_AVG) ? bands->midSmooth / bands->midAvg : 0.0f;
    const float trebNorm = (bands->trebAvg > MIN_AVG) ? bands->trebSmooth / bands->trebAvg : 0.0f;

    json msg = {
        {"type", "analysis"},
        {"beat", beatIntensity},
        {"bass", bassNorm},
        {"mid", midNorm},
        {"treb", trebNorm}
    };

    SerializeJsonToBuffer(msg, outJson, maxLen);
}

bool WebBridgeApplyCommand(AppConfigs* configs, const char* jsonStr)
{
    if (configs == NULL || jsonStr == NULL) {
        return false;
    }

    try {
        json msg = json::parse(jsonStr);

        if (!msg.contains("cmd")) {
            return false;
        }

        const std::string cmd = msg["cmd"].get<std::string>();

        if (cmd == "setAudioChannel") {
            if (!msg.contains("value")) {
                return false;
            }
            int value = msg["value"].get<int>();
            if (value < 0 || value > 5) {
                return false;
            }
            configs->audio->channelMode = (ChannelMode)value;
            return true;
        }

        if (cmd == "presetList") {
            return true;
        }

        if (cmd == "presetLoad") {
            if (!msg.contains("filename")) {
                return false;
            }
            std::string filename = msg["filename"].get<std::string>();
            if (!IsValidPresetFilename(filename.c_str())) {
                return false;
            }
            char filepath[PRESET_PATH_MAX];
            BuildPresetPath(filepath, sizeof(filepath), filename.c_str());
            Preset p = {};
            if (!PresetLoad(&p, filepath)) {
                return false;
            }
            PresetToAppConfigs(&p, configs);
            return true;
        }

        if (cmd == "presetSave") {
            if (!msg.contains("name")) {
                return false;
            }
            std::string name = msg["name"].get<std::string>();
            if (name.empty() || !IsValidPresetFilename(name.c_str())) {
                return false;
            }
            char filepath[PRESET_PATH_MAX];
            snprintf(filepath, sizeof(filepath), "presets/%s.json", name.c_str());
            Preset p = {};
            strncpy(p.name, name.c_str(), PRESET_NAME_MAX - 1);
            p.name[PRESET_NAME_MAX - 1] = '\0';
            PresetFromAppConfigs(&p, configs);
            return PresetSave(&p, filepath);
        }

        if (cmd == "presetDelete") {
            if (!msg.contains("filename")) {
                return false;
            }
            std::string filename = msg["filename"].get<std::string>();
            if (!IsValidPresetFilename(filename.c_str())) {
                return false;
            }
            char filepath[PRESET_PATH_MAX];
            BuildPresetPath(filepath, sizeof(filepath), filename.c_str());
            std::error_code ec;
            return std::filesystem::remove(filepath, ec);
        }

        return false;
    }
    catch (const json::exception& e) {
        return false;
    }
}

void WebBridgeSerializeConfig(const AppConfigs* configs,
                               char* outJson, int maxLen)
{
    if (configs == NULL) {
        SerializeJsonToBuffer(json({}), outJson, maxLen);
        return;
    }

    Preset p = {};
    p.name[0] = '\0';
    PresetFromAppConfigs(&p, configs);

    json msg = {
        {"type", "config"},
        {"preset", p}
    };

    SerializeJsonToBuffer(msg, outJson, maxLen);
}

void WebBridgeSerializePresetStatus(bool success,
                                     const char* message,
                                     char outFiles[][PRESET_PATH_MAX],
                                     int fileCount,
                                     char* outJson,
                                     int maxLen)
{
    json presetArray = json::array();
    for (int i = 0; i < fileCount; i++) {
        presetArray.push_back(outFiles[i]);
    }

    json msg = {
        {"type", "presetStatus"},
        {"success", success},
        {"message", message ? message : ""},
        {"presets", presetArray}
    };

    SerializeJsonToBuffer(msg, outJson, maxLen);
}
