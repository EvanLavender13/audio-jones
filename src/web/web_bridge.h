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

#endif // WEB_BRIDGE_H
