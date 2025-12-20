#include "web_bridge.h"

#include <stdio.h>
#include "nlohmann/json.hpp"
#include "config/app_configs.h"
#include "analysis/beat.h"
#include "analysis/bands.h"

using json = nlohmann::json;

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

    json msg = {
        {"type", "config"},
        {"audio", {
            {"channelMode", (int)configs->audio->channelMode}
        }}
    };

    SerializeJsonToBuffer(msg, outJson, maxLen);
}
