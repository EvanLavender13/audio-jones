#ifndef APP_CONFIGS_H
#define APP_CONFIGS_H

#include "config/drawable_config.h"
#include "audio/audio_config.h"
#include "config/effect_config.h"
#include "analysis/beat.h"
#include "analysis/bands.h"
#include "config/lfo_config.h"

typedef struct PostEffect PostEffect;

struct AppConfigs {
    Drawable* drawables;
    int* drawableCount;
    int* selectedDrawable;
    EffectConfig* effects;
    AudioConfig* audio;
    BeatDetector* beat;
    BandEnergies* bandEnergies;
    LFOConfig* lfos;
    PostEffect* postEffect;
};

#endif // APP_CONFIGS_H
