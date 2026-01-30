#ifndef RADIAL_STREAK_CONFIG_H
#define RADIAL_STREAK_CONFIG_H

// Radial Blur / Zoom Blur
// Samples along radial lines toward center, creating motion blur streaks
struct RadialStreakConfig {
  bool enabled = false;
  int samples = 16;          // Sample count (8-32)
  float streakLength = 0.3f; // Streak distance factor (0.1-1.0)
};

#endif // RADIAL_STREAK_CONFIG_H
