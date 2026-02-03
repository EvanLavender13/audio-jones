// src/config/anamorphic_streak_config.h
#ifndef ANAMORPHIC_STREAK_CONFIG_H
#define ANAMORPHIC_STREAK_CONFIG_H

struct AnamorphicStreakConfig {
  bool enabled = false;
  float threshold = 0.8f; // Brightness cutoff (0.0-2.0)
  float knee = 0.5f;      // Soft threshold falloff (0.0-1.0)
  float intensity = 0.5f; // Streak brightness in composite (0.0-2.0)
  float stretch = 8.0f;   // Horizontal extent multiplier (1.0-20.0)
  float sharpness =
      0.3f;           // Kernel falloff: soft (0) to hard lines (1) (0.0-1.0)
  int iterations = 4; // Blur pass count (2-6)
};

#endif // ANAMORPHIC_STREAK_CONFIG_H
