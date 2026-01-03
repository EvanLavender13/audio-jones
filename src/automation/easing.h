#ifndef EASING_H
#define EASING_H

#include <math.h>

static const float EASING_PI = 3.14159265358979323846f;

// Easing functions for modulation curves.
// All functions take t in [0,1] and return eased value.
// Spring/elastic/bounce may return values outside [0,1] (overshoot).

inline float EaseInCubic(float t) {
    return t * t * t;
}

inline float EaseOutCubic(float t) {
    const float inv = 1.0f - t;
    return 1.0f - (inv * inv * inv);
}

inline float EaseInOutCubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    const float inv = -2.0f * t + 2.0f;
    return 1.0f - (inv * inv * inv) / 2.0f;
}

inline float EaseSpring(float t) {
    // Damped oscillation: 1 - cos(t*π*2.5) * e^(-6t)
    return 1.0f - cosf(t * EASING_PI * 2.5f) * expf(-6.0f * t);
}

inline float EaseElastic(float t) {
    // Sine with exponential decay for overshoot effect
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return 1.0f - cosf(t * EASING_PI * 2.0f) * expf(-4.0f * t);
}

inline float EaseBounce(float t) {
    // Piecewise parabolic bounces
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    }
    if (t < 2.0f / d1) {
        const float t2 = t - 1.5f / d1;
        return n1 * t2 * t2 + 0.75f;
    }
    if (t < 2.5f / d1) {
        const float t2 = t - 2.25f / d1;
        return n1 * t2 * t2 + 0.9375f;
    }
    const float t2 = t - 2.625f / d1;
    return n1 * t2 * t2 + 0.984375f;
}

// Unipolar curve evaluation (t in [0,1]) for preview rendering.
// curve: ModCurve enum value (0=Linear, 1=EaseIn, ... 6=Bounce)
inline float EasingEvaluate(float t, int curve)
{
    switch (curve) {
        case 0: return t;                   // LINEAR
        case 1: return EaseInCubic(t);      // EASE_IN
        case 2: return EaseOutCubic(t);     // EASE_OUT
        case 3: return EaseInOutCubic(t);   // EASE_IN_OUT
        case 4: return EaseSpring(t);       // SPRING
        case 5: return EaseElastic(t);      // ELASTIC
        case 6: return EaseBounce(t);       // BOUNCE
        default: return t;
    }
}

#endif // EASING_H
