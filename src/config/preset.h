#ifndef PRESET_H
#define PRESET_H

#include "audio/audio_config.h"
#include "drawable_config.h"
#include "effect_config.h"
#include "lfo_config.h"
#include "modulation_config.h"
#include "render/drawable.h"
#include <stdbool.h>

#define PRESET_NAME_MAX 64
#define PRESET_PATH_MAX 256
#define MAX_PRESET_ENTRIES 128
#define PRESET_DIR "presets"

struct PresetEntry {
  char name[PRESET_PATH_MAX]; // Display name (folder name or filename without
                              // .json)
  bool isFolder;              // true = directory, false = .json preset
};

struct Preset {
  char name[PRESET_NAME_MAX];
  EffectConfig effects;
  AudioConfig audio;
  Drawable drawables[MAX_DRAWABLES];
  int drawableCount;
  ModulationConfig modulation;
  LFOConfig lfos[NUM_LFOS];
};

// Initialize preset with defaults
Preset PresetDefault(void);

// Save preset to file. Returns true on success.
bool PresetSave(const Preset *preset, const char *filepath);

// Load preset from file. Returns true on success.
bool PresetLoad(Preset *preset, const char *filepath);

// List entries (folders + .json presets) in directory.
// Entries sorted: folders first (case-insensitive alpha), then presets
// (case-insensitive alpha). Returns number of entries found.
int PresetListEntries(const char *directory, PresetEntry *entries,
                      int maxEntries);

// Forward declaration for conversion helpers
struct AppConfigs;

// Copy app config values into preset (for saving)
void PresetFromAppConfigs(Preset *preset, const struct AppConfigs *configs);

// Copy preset values into app configs (for loading)
void PresetToAppConfigs(const Preset *preset, struct AppConfigs *configs);

#endif // PRESET_H
