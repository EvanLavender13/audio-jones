#ifndef LOG_POLAR_SPIRAL_CONFIG_H
#define LOG_POLAR_SPIRAL_CONFIG_H

struct LogPolarSpiralConfig {
    bool enabled = false;
    float speed = 1.0f;          // Animation speed (0.1-2.0)
    float zoomDepth = 3.0f;      // Zoom range in powers of 2 (1.0-5.0)
    float focalAmplitude = 0.0f; // Lissajous center offset (0.0-0.2)
    float focalFreqX = 1.0f;     // Lissajous X frequency (0.1-5.0)
    float focalFreqY = 1.5f;     // Lissajous Y frequency (0.1-5.0)
    int layers = 6;              // Layer count (2-8)
    float spiralTwist = 0.0f;    // Angle varies with log-radius (radians)
    float spiralTurns = 0.0f;    // Rotation per zoom cycle (turns)
};

#endif // LOG_POLAR_SPIRAL_CONFIG_H
