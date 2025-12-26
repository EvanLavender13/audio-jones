#ifndef APP_CONFIGS_H
#define APP_CONFIGS_H

#include "config/waveform_config.h"
#include "audio/audio_config.h"
#include "config/effect_config.h"
#include "config/spectrum_bars_config.h"
#include "analysis/beat.h"
#include "analysis/bands.h"

struct AppConfigs {
    WaveformConfig* waveforms;
    int* waveformCount;
    int* selectedWaveform;
    EffectConfig* effects;
    AudioConfig* audio;
    SpectrumConfig* spectrum;
    BeatDetector* beat;
    BandEnergies* bandEnergies;
};

#endif // APP_CONFIGS_H
