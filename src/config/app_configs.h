#ifndef APP_CONFIGS_H
#define APP_CONFIGS_H

#include "analysis/bands.h"
#include "analysis/beat.h"
#include "audio/audio_config.h"
#include "config/drawable_config.h"
#include "config/effect_config.h"
#include "config/lfo_config.h"
#include "config/mod_bus_config.h"

typedef struct PostEffect PostEffect;

struct AppConfigs {
  Drawable *drawables;
  int *drawableCount;
  int *selectedDrawable;
  EffectConfig *effects;
  AudioConfig *audio;
  BeatDetector *beat;
  BandEnergies *bandEnergies;
  LFOConfig *lfos;
  ModBusConfig *modBuses;
  PostEffect *postEffect;
};

#endif // APP_CONFIGS_H
