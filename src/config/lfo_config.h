#ifndef LFO_CONFIG_H
#define LFO_CONFIG_H

#define NUM_LFOS 8

typedef enum {
  LFO_WAVE_SINE,
  LFO_WAVE_TRIANGLE,
  LFO_WAVE_SAWTOOTH,
  LFO_WAVE_SQUARE,
  LFO_WAVE_SAMPLE_HOLD,
  LFO_WAVE_SMOOTH_RANDOM,
  LFO_WAVE_COUNT
} LFOWaveform;

struct LFOConfig {
  bool enabled = false;
  float rate = 0.1f; // Oscillation frequency (Hz)
  int waveform = LFO_WAVE_SINE;
};

#endif // LFO_CONFIG_H
