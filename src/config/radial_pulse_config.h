#ifndef RADIAL_PULSE_CONFIG_H
#define RADIAL_PULSE_CONFIG_H

// Radial Pulse: polar-native sine distortion
// Creates concentric rings, petal symmetry, and animated spirals via
// radial/angular displacement
struct RadialPulseConfig {
  bool enabled = false;
  float radialFreq = 8.0f;  // Ring density (1.0-30.0)
  float radialAmp = 0.05f;  // Radial displacement strength (±0.3)
  int segments = 6;         // Petal count (2-16)
  float angularAmp = 0.1f;  // Tangential swirl strength (±0.5)
  float petalAmp = 0.0f;    // Radial petal modulation (±1.0)
  float phaseSpeed = 1.0f;  // Animation speed (±5.0 rad/sec)
  float spiralTwist = 0.0f; // Angular phase shift per radius (radians)
};

#endif // RADIAL_PULSE_CONFIG_H
