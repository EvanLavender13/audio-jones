// src/config/relativistic_doppler_config.h
#ifndef RELATIVISTIC_DOPPLER_CONFIG_H
#define RELATIVISTIC_DOPPLER_CONFIG_H

struct RelativisticDopplerConfig {
  bool enabled = false;
  float velocity = 0.5f;   // 0.0 - 0.99, fraction of light speed
  float centerX = 0.5f;    // 0.0 - 1.0, travel direction X
  float centerY = 0.5f;    // 0.0 - 1.0, travel direction Y
  float aberration = 1.0f; // 0.0 - 1.0, spatial compression strength
  float colorShift = 1.0f; // 0.0 - 1.0, Doppler hue intensity
  float headlight = 0.3f;  // 0.0 - 1.0, brightness boost toward center
};

#endif // RELATIVISTIC_DOPPLER_CONFIG_H
