#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "simulation/physarum.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"
#include "kaleidoscope_config.h"
#include "voronoi_config.h"
#include "infinite_zoom_config.h"
#include "mobius_config.h"
#include "turbulence_config.h"
#include "radial_streak_config.h"
#include "multi_inversion_config.h"

enum TransformEffectType {
    TRANSFORM_MOBIUS = 0,
    TRANSFORM_TURBULENCE,
    TRANSFORM_KALEIDOSCOPE,
    TRANSFORM_INFINITE_ZOOM,
    TRANSFORM_RADIAL_STREAK,
    TRANSFORM_MULTI_INVERSION,
    TRANSFORM_EFFECT_COUNT
};

inline const char* TransformEffectName(TransformEffectType type) {
    switch (type) {
        case TRANSFORM_MOBIUS:            return "Mobius";
        case TRANSFORM_TURBULENCE:        return "Turbulence";
        case TRANSFORM_KALEIDOSCOPE:      return "Kaleidoscope";
        case TRANSFORM_INFINITE_ZOOM:     return "Infinite Zoom";
        case TRANSFORM_RADIAL_STREAK:     return "Radial Streak";
        case TRANSFORM_MULTI_INVERSION:   return "Multi-Inversion";
        default:                          return "Unknown";
    }
}

struct FlowFieldConfig {
    float zoomBase = 0.995f;
    float zoomRadial = 0.0f;
    float rotBase = 0.0f;
    float rotRadial = 0.0f;
    float dxBase = 0.0f;
    float dxRadial = 0.0f;
    float dyBase = 0.0f;
    float dyRadial = 0.0f;
};

struct EffectConfig {
    float halfLife = 0.5f;           // Trail persistence (seconds)
    float blurScale = 1.0f;          // Blur sampling distance (pixels)
    float chromaticOffset = 0.0f;    // RGB channel offset (pixels, 0 = disabled)
    float feedbackDesaturate = 0.05f;// Fade toward dark gray per frame (0.0-0.2)
    FlowFieldConfig flowField;       // Spatial UV flow field parameters
    float gamma = 1.0f;              // Display gamma correction (1.0 = disabled)
    float clarity = 0.0f;            // Local contrast enhancement (0.0 = disabled)

    // Kaleidoscope effect
    KaleidoscopeConfig kaleidoscope;

    // Voronoi effect
    VoronoiConfig voronoi;

    // Physarum simulation
    PhysarumConfig physarum;

    // Curl noise flow
    CurlFlowConfig curlFlow;

    // Infinite zoom
    InfiniteZoomConfig infiniteZoom;

    // Strange attractor flow
    AttractorFlowConfig attractorFlow;

    // Möbius transformation
    MobiusConfig mobius;

    // Turbulence cascade
    TurbulenceConfig turbulence;

    // Radial streak
    RadialStreakConfig radialStreak;

    // Multi-inversion blend
    MultiInversionConfig multiInversion;

    // Transform effect execution order
    TransformEffectType transformOrder[TRANSFORM_EFFECT_COUNT] = {
        TRANSFORM_MOBIUS,
        TRANSFORM_TURBULENCE,
        TRANSFORM_KALEIDOSCOPE,
        TRANSFORM_INFINITE_ZOOM,
        TRANSFORM_RADIAL_STREAK,
        TRANSFORM_MULTI_INVERSION
    };
};

#endif // EFFECT_CONFIG_H
