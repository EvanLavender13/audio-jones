#ifndef PRESET_H
#define PRESET_H

#include "render/waveform.h"
#include "audio/audio_config.h"
#include "effect_config.h"
#include "spectrum_bars_config.h"
#include "band_config.h"
#include <stdbool.h>
#include <nlohmann/json_fwd.hpp>

#define PRESET_NAME_MAX 64
#define PRESET_PATH_MAX 256
#define MAX_PRESET_FILES 32

struct Preset {
    char name[PRESET_NAME_MAX];
    EffectConfig effects;
    AudioConfig audio;
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
    SpectrumConfig spectrum;
    BandConfig bands;
};

void to_json(nlohmann::json& j, const Preset& p);
void from_json(const nlohmann::json& j, Preset& p);

// Initialize preset with defaults
Preset PresetDefault(void);

// Save preset to file. Returns true on success.
bool PresetSave(const Preset* preset, const char* filepath);

// Load preset from file. Returns true on success.
bool PresetLoad(Preset* preset, const char* filepath);

// List available preset files in directory.
// Fills outFiles array with filenames (without path).
// Returns number of files found.
int PresetListFiles(const char* directory, char outFiles[][PRESET_PATH_MAX], int maxFiles);

// Forward declaration for conversion helpers
struct AppConfigs;

// Copy app config values into preset (for saving)
void PresetFromAppConfigs(Preset* preset, const struct AppConfigs* configs);

// Copy preset values into app configs (for loading)
void PresetToAppConfigs(const Preset* preset, struct AppConfigs* configs);

#endif // PRESET_H
