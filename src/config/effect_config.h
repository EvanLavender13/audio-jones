#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "simulation/physarum.h"
#include "simulation/curl_flow.h"
#include "simulation/attractor_flow.h"
#include "kaleidoscope_config.h"
#include "kifs_config.h"
#include "lattice_fold_config.h"
#include "voronoi_config.h"
#include "infinite_zoom_config.h"
#include "sine_warp_config.h"
#include "radial_streak_config.h"
#include "texture_warp_config.h"
#include "wave_ripple_config.h"
#include "mobius_config.h"
#include "pixelation_config.h"
#include "glitch_config.h"
#include "poincare_disk_config.h"
#include "toon_config.h"
#include "heightfield_relief_config.h"
#include "gradient_flow_config.h"
#include "droste_zoom_config.h"
#include "color_grade_config.h"
#include "ascii_art_config.h"
#include "oil_paint_config.h"

enum TransformEffectType {
    TRANSFORM_SINE_WARP = 0,
    TRANSFORM_KALEIDOSCOPE,
    TRANSFORM_INFINITE_ZOOM,
    TRANSFORM_RADIAL_STREAK,
    TRANSFORM_TEXTURE_WARP,
    TRANSFORM_VORONOI,
    TRANSFORM_WAVE_RIPPLE,
    TRANSFORM_MOBIUS,
    TRANSFORM_PIXELATION,
    TRANSFORM_GLITCH,
    TRANSFORM_POINCARE_DISK,
    TRANSFORM_TOON,
    TRANSFORM_HEIGHTFIELD_RELIEF,
    TRANSFORM_GRADIENT_FLOW,
    TRANSFORM_DROSTE_ZOOM,
    TRANSFORM_KIFS,
    TRANSFORM_LATTICE_FOLD,
    TRANSFORM_COLOR_GRADE,
    TRANSFORM_ASCII_ART,
    TRANSFORM_OIL_PAINT,
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
        case TRANSFORM_PIXELATION:        return "Pixelation";
        case TRANSFORM_GLITCH:            return "Glitch";
        case TRANSFORM_POINCARE_DISK:     return "Poincare Disk";
        case TRANSFORM_TOON:              return "Toon";
        case TRANSFORM_HEIGHTFIELD_RELIEF: return "Heightfield Relief";
        case TRANSFORM_GRADIENT_FLOW:     return "Gradient Flow";
        case TRANSFORM_DROSTE_ZOOM:       return "Droste Zoom";
        case TRANSFORM_KIFS:              return "KIFS";
        case TRANSFORM_LATTICE_FOLD:      return "Lattice Fold";
        case TRANSFORM_COLOR_GRADE:       return "Color Grade";
        case TRANSFORM_ASCII_ART:         return "ASCII Art";
        case TRANSFORM_OIL_PAINT:         return "Oil Paint";
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
        TRANSFORM_MOBIUS,
        TRANSFORM_PIXELATION,
        TRANSFORM_GLITCH,
        TRANSFORM_POINCARE_DISK,
        TRANSFORM_TOON,
        TRANSFORM_HEIGHTFIELD_RELIEF,
        TRANSFORM_GRADIENT_FLOW,
        TRANSFORM_DROSTE_ZOOM,
        TRANSFORM_KIFS,
        TRANSFORM_LATTICE_FOLD,
        TRANSFORM_COLOR_GRADE,
        TRANSFORM_ASCII_ART,
        TRANSFORM_OIL_PAINT
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

    // Kaleidoscope effect (Polar mirroring)
    KaleidoscopeConfig kaleidoscope;

    // KIFS (Kaleidoscopic IFS fractal folding)
    KifsConfig kifs;

    // Lattice Fold (grid-based tiling symmetry)
    LatticeFoldConfig latticeFold;

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

    // Pixelation (UV quantization with dither/posterize)
    PixelationConfig pixelation;

    // Glitch (CRT, Analog, Digital, VHS video corruption)
    GlitchConfig glitch;

    // Poincare Disk (hyperbolic tiling)
    PoincareDiskConfig poincareDisk;

    // Toon (cartoon posterization with edge outlines)
    ToonConfig toon;

    // Heightfield Relief (embossed lighting from luminance gradients)
    HeightfieldReliefConfig heightfieldRelief;

    // Gradient Flow (edge-following UV displacement)
    GradientFlowConfig gradientFlow;

    // Droste Zoom (conformal log-polar recursive zoom)
    DrosteZoomConfig drosteZoom;

    // Color Grade (full-spectrum color manipulation)
    ColorGradeConfig colorGrade;

    // ASCII Art (luminance-based character rendering)
    AsciiArtConfig asciiArt;

    // Oil Paint (4-sector Kuwahara painterly filter)
    OilPaintConfig oilPaint;

    // Transform effect execution order
    TransformOrderConfig transformOrder;
};

#endif // EFFECT_CONFIG_H
