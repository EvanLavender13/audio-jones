#ifndef RADIAL_STREAK_CONFIG_H
#define RADIAL_STREAK_CONFIG_H

// Radial Streak Accumulation
// Samples along radial/spiral path from center, creating motion-blur-like streaks
struct RadialStreakConfig {
    bool enabled = false;
    int samples = 16;            // Sample count (8-32)
    float streakLength = 0.3f;   // Streak distance factor (0.1-1.0)
    float spiralTwist = 0.0f;    // Rotation per sample (radians, 0 = pure radial)
    float focalAmplitude = 0.0f; // Lissajous center offset (UV units, 0 = static center)
    float focalFreqX = 1.0f;     // Lissajous X frequency
    float focalFreqY = 1.5f;     // Lissajous Y frequency
};

#endif // RADIAL_STREAK_CONFIG_H
