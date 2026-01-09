#ifndef MOBIUS_CONFIG_H
#define MOBIUS_CONFIG_H

// Mobius Transform: conformal UV warp via complex-plane fractional linear transformation
// Two fixed points define geometry; rho (expansion) and alpha (rotation) control character
struct MobiusConfig {
    bool enabled = false;
    float point1X = 0.3f;         // Fixed point 1 X in UV space (0.0-1.0)
    float point1Y = 0.5f;         // Fixed point 1 Y (0.0-1.0)
    float point2X = 0.7f;         // Fixed point 2 X in UV space (0.0-1.0)
    float point2Y = 0.5f;         // Fixed point 2 Y (0.0-1.0)
    float rho = 0.0f;             // Hyperbolic strength: expansion rate (-2.0-2.0)
    float alpha = 0.0f;           // Elliptic strength: rotation rate in radians (-PI-PI)
    float animSpeed = 1.0f;       // Time multiplier for rho/alpha animation (0.0-2.0)
    float pointAmplitude = 0.0f;  // Lissajous motion amplitude (0.0-0.3)
    float pointFreq1 = 1.0f;      // Point 1 oscillation frequency (0.1-5.0)
    float pointFreq2 = 1.3f;      // Point 2 oscillation frequency (0.1-5.0)
};

#endif // MOBIUS_CONFIG_H
