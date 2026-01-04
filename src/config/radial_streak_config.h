#ifndef RADIAL_STREAK_CONFIG_H
#define RADIAL_STREAK_CONFIG_H

// Radial Streak Accumulation
// Samples along radial or spiral paths from center, creating motion-blur-like streaks
struct RadialStreakConfig {
    bool enabled = false;
    int mode = 0;                // 0=radial, 1=spiral
    int samples = 8;             // Sample count (4-16)
    float streakLength = 0.3f;   // Maximum streak distance in UV units (0.1-1.0)
    float spiralTwist = 0.0f;    // Additional angle per sample (radians)
    float spiralTurns = 0.0f;    // Base rotation offset (radians)
    float focalAmplitude = 0.0f; // Lissajous center offset (UV units, 0 = static center)
    float focalFreqX = 1.0f;     // Lissajous X frequency
    float focalFreqY = 1.5f;     // Lissajous Y frequency
};

#endif // RADIAL_STREAK_CONFIG_H
