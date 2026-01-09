#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "simulation/physarum.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"
#include "kaleidoscope_config.h"
#include "voronoi_config.h"
#include "infinite_zoom_config.h"
#include "sine_warp_config.h"
#include "radial_streak_config.h"
#include "texture_warp_config.h"
#include "wave_ripple_config.h"
#include "mobius_config.h"

enum TransformEffectType {
    TRANSFORM_SINE_WARP = 0,
    TRANSFORM_KALEIDOSCOPE,
    TRANSFORM_INFINITE_ZOOM,
    TRANSFORM_RADIAL_STREAK,
    TRANSFORM_TEXTURE_WARP,
    TRANSFORM_VORONOI,
    TRANSFORM_WAVE_RIPPLE,
    TRANSFORM_MOBIUS,
    TRANSFORM_EFFECT_COUNT
};

inline const char* TransformEffectName(TransformEffectType type) {
    switch (type) {
        case TRANSFORM_SINE_WARP:         return "Sine Warp";
        case TRANSFORM_KALEIDOSCOPE:      return "Kaleidoscope";
        case TRANSFORM_INFINITE_ZOOM:     return "Infinite Zoom";
        case TRANSFORM_RADIAL_STREAK:     return "Radial Blur";
        case TRANSFORM_TEXTURE_WARP:      return "Texture Warp";
        case TRANSFORM_VORONOI:           return "Voronoi";
        case TRANSFORM_WAVE_RIPPLE:       return "Wave Ripple";
        case TRANSFORM_MOBIUS:            return "Mobius";
        default:                          return "Unknown";
    }
}

struct TransformOrderConfig {
    TransformEffectType order[TRANSFORM_EFFECT_COUNT] = {
        TRANSFORM_SINE_WARP,
        TRANSFORM_KALEIDOSCOPE,
        TRANSFORM_INFINITE_ZOOM,
        TRANSFORM_RADIAL_STREAK,
        TRANSFORM_TEXTURE_WARP,
        TRANSFORM_VORONOI,
        TRANSFORM_WAVE_RIPPLE,
        TRANSFORM_MOBIUS
    };

    TransformEffectType& operator[](int i) { return order[i]; }
    const TransformEffectType& operator[](int i) const { return order[i]; }
};

struct FlowFieldConfig {
    float zoomBase = 0.995f;
    float zoomRadial = 0.0f;
    float rotationSpeed = 0.0f;
    float rotationSpeedRadial = 0.0f;
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

    // Sine warp
    SineWarpConfig sineWarp;

    // Radial blur
    RadialStreakConfig radialStreak;

    // Texture warp (self-referential distortion)
    TextureWarpConfig textureWarp;

    // Wave ripple (pseudo-3D radial waves)
    WaveRippleConfig waveRipple;

    // Mobius transform (conformal UV warp)
    MobiusConfig mobius;

    // Transform effect execution order
    TransformOrderConfig transformOrder;
};

#endif // EFFECT_CONFIG_H
