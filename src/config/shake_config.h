// src/config/shake_config.h
#ifndef SHAKE_CONFIG_H
#define SHAKE_CONFIG_H

struct ShakeConfig {
  bool enabled = false;
  float intensity = 0.02f; // UV displacement distance (0.0 - 0.2)
  float samples = 4.0f;    // Samples per pixel (1 - 16), float for modulation
  float rate = 12.0f;      // Jitter change frequency in Hz (1.0 - 60.0)
  bool gaussian = false;   // false = uniform distribution, true = gaussian
};

#endif // SHAKE_CONFIG_H
