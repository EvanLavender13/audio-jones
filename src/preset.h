#ifndef PRESET_H
#define PRESET_H

#include "waveform.h"
#include "audio_config.h"
#include "effects_config.h"
#include <stdbool.h>

#define PRESET_NAME_MAX 64
#define PRESET_PATH_MAX 256
#define MAX_PRESET_FILES 32

typedef struct {
    char name[PRESET_NAME_MAX];
    EffectsConfig effects;
    AudioConfig audio;
    WaveformConfig waveforms[MAX_WAVEFORMS];
    int waveformCount;
} Preset;

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

#endif // PRESET_H
