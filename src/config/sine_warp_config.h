#ifndef SINE_WARP_CONFIG_H
#define SINE_WARP_CONFIG_H

// Sine Warp with depth accumulation
// Stacks sine-based coordinate shifts with rotation to create organic swirl
// patterns. Supports Cartesian or Polar (radial) coordinate modes.
// depthBlend samples color at each octave for layered effect; disable for
// single final sample.
struct SineWarpConfig {
  bool enabled = false;
  int octaves = 4;             // Number of cascade octaves (1-8)
  float strength = 0.5f;       // Distortion intensity (0.0-2.0)
  float animRate = 0.3f;       // Animation rate (radians/second, 0.0-2.0)
  float octaveRotation = 0.5f; // Rotation per octave in radians (±π)
  bool radialMode = false;     // false=Cartesian warp, true=Polar warp
  bool depthBlend = true;      // true=sample each octave, false=sample once
};

#endif // SINE_WARP_CONFIG_H
