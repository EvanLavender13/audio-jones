#ifndef WEB_BRIDGE_H
#define WEB_BRIDGE_H

#include <stdbool.h>

// Forward declarations to avoid raylib/Windows header conflicts
struct AppConfigs;
struct BeatDetector;
struct BandEnergies;

// Serialize analysis state to JSON
// outJson: output buffer, maxLen: buffer size
void WebBridgeSerializeAnalysis(const BeatDetector* beat,
                                 const BandEnergies* bands,
                                 char* outJson, int maxLen);

// Parse and apply config command from JSON
// Returns true if valid command was applied
bool WebBridgeApplyCommand(AppConfigs* configs, const char* json);

// Serialize current config to JSON (for new client sync)
void WebBridgeSerializeConfig(const AppConfigs* configs,
                               char* outJson, int maxLen);

// Serialize preset operation status with current file list
// PRESET_PATH_MAX defined here for files that can't include preset.h
#ifndef PRESET_PATH_MAX
#define PRESET_PATH_MAX 256
#endif
void WebBridgeSerializePresetStatus(bool success,
                                     const char* message,
                                     char outFiles[][PRESET_PATH_MAX],
                                     int fileCount,
                                     char* outJson,
                                     int maxLen);

#endif // WEB_BRIDGE_H
