// Displacement waveform modes for domain warping and FBM turbulence
#ifndef WAVEFORM_MODE_H
#define WAVEFORM_MODE_H

typedef enum {
  WAVEFORM_SINE = 0,    // Smooth swirling folds
  WAVEFORM_FRACT_FOLD,  // Crystalline/faceted (sawtooth)
  WAVEFORM_ABS_SIN,     // Sharp valleys, always-positive
  WAVEFORM_TRIANGLE,    // Zigzag between sin and fract character
  WAVEFORM_SQUARED_SIN, // Soft peaks, flat valleys
  WAVEFORM_SQUARE_WAVE, // Blocky digital geometry
  WAVEFORM_QUANTIZED,   // Staircase structures
  WAVEFORM_COSINE,      // Phase-shifted sine, different fold geometry
  WAVEFORM_COUNT
} WaveformMode;

#endif // WAVEFORM_MODE_H
