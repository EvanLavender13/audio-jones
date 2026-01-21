#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "simulation/physarum.h"
#include "simulation/curl_flow.h"
#include "simulation/curl_advection.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/cymatics.h"
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
#include "watercolor_config.h"
#include "neon_glow_config.h"
#include "radial_pulse_config.h"
#include "false_color_config.h"
#include "halftone_config.h"
#include "feedback_flow_config.h"
#include "procedural_warp_config.h"
#include "chladni_warp_config.h"
#include "cross_hatching_config.h"
#include "palette_quantization_config.h"
#include "bokeh_config.h"
#include "bloom_config.h"
#include "mandelbox_config.h"
#include "triangle_fold_config.h"
#include "domain_warp_config.h"
#include "phyllotaxis_config.h"
#include "phyllotaxis_warp_config.h"
#include "density_wave_spiral_config.h"
#include "moire_interference_config.h"

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
    TRANSFORM_WATERCOLOR,
    TRANSFORM_NEON_GLOW,
    TRANSFORM_RADIAL_PULSE,
    TRANSFORM_FALSE_COLOR,
    TRANSFORM_HALFTONE,
    TRANSFORM_CHLADNI_WARP,
    TRANSFORM_CROSS_HATCHING,
    TRANSFORM_PALETTE_QUANTIZATION,
    TRANSFORM_BOKEH,
    TRANSFORM_BLOOM,
    TRANSFORM_MANDELBOX,
    TRANSFORM_TRIANGLE_FOLD,
    TRANSFORM_DOMAIN_WARP,
    TRANSFORM_PHYLLOTAXIS,
    TRANSFORM_PHYLLOTAXIS_WARP,
    TRANSFORM_PHYSARUM_BOOST,
    TRANSFORM_CURL_FLOW_BOOST,
    TRANSFORM_CURL_ADVECTION_BOOST,
    TRANSFORM_ATTRACTOR_FLOW_BOOST,
    TRANSFORM_BOIDS_BOOST,
    TRANSFORM_CYMATICS_BOOST,
    TRANSFORM_DENSITY_WAVE_SPIRAL,
    TRANSFORM_MOIRE_INTERFERENCE,
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
        case TRANSFORM_WATERCOLOR:        return "Watercolor";
        case TRANSFORM_NEON_GLOW:         return "Neon Glow";
        case TRANSFORM_RADIAL_PULSE:      return "Radial Pulse";
        case TRANSFORM_FALSE_COLOR:           return "False Color";
        case TRANSFORM_HALFTONE:          return "Halftone";
        case TRANSFORM_CHLADNI_WARP:      return "Chladni Warp";
        case TRANSFORM_CROSS_HATCHING:    return "Cross-Hatching";
        case TRANSFORM_PALETTE_QUANTIZATION: return "Palette Quantization";
        case TRANSFORM_BOKEH:             return "Bokeh";
        case TRANSFORM_BLOOM:             return "Bloom";
        case TRANSFORM_MANDELBOX:         return "Mandelbox";
        case TRANSFORM_TRIANGLE_FOLD:     return "Triangle Fold";
        case TRANSFORM_DOMAIN_WARP:       return "Domain Warp";
        case TRANSFORM_PHYLLOTAXIS:       return "Phyllotaxis";
        case TRANSFORM_PHYLLOTAXIS_WARP:  return "Phyllotaxis Warp";
        case TRANSFORM_PHYSARUM_BOOST:    return "Physarum Boost";
        case TRANSFORM_CURL_FLOW_BOOST:   return "Curl Flow Boost";
        case TRANSFORM_CURL_ADVECTION_BOOST: return "Curl Advection Boost";
        case TRANSFORM_ATTRACTOR_FLOW_BOOST: return "Attractor Flow Boost";
        case TRANSFORM_BOIDS_BOOST:       return "Boids Boost";
        case TRANSFORM_CYMATICS_BOOST:    return "Cymatics Boost";
        case TRANSFORM_DENSITY_WAVE_SPIRAL: return "Density Wave Spiral";
        case TRANSFORM_MOIRE_INTERFERENCE: return "Moire Interference";
        default:                          return "Unknown";
    }
}

// Forward declaration for IsTransformEnabled
struct EffectConfig;

// Check if a transform effect is enabled in the config
inline bool IsTransformEnabled(const EffectConfig* e, TransformEffectType type);

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
        TRANSFORM_OIL_PAINT,
        TRANSFORM_WATERCOLOR,
        TRANSFORM_NEON_GLOW,
        TRANSFORM_RADIAL_PULSE,
        TRANSFORM_FALSE_COLOR,
        TRANSFORM_HALFTONE,
        TRANSFORM_CHLADNI_WARP,
        TRANSFORM_CROSS_HATCHING,
        TRANSFORM_PALETTE_QUANTIZATION,
        TRANSFORM_BOKEH,
        TRANSFORM_BLOOM,
        TRANSFORM_MANDELBOX,
        TRANSFORM_TRIANGLE_FOLD,
        TRANSFORM_DOMAIN_WARP,
        TRANSFORM_PHYLLOTAXIS,
        TRANSFORM_PHYLLOTAXIS_WARP,
        TRANSFORM_PHYSARUM_BOOST,
        TRANSFORM_CURL_FLOW_BOOST,
        TRANSFORM_CURL_ADVECTION_BOOST,
        TRANSFORM_ATTRACTOR_FLOW_BOOST,
        TRANSFORM_BOIDS_BOOST,
        TRANSFORM_CYMATICS_BOOST,
        TRANSFORM_DENSITY_WAVE_SPIRAL,
        TRANSFORM_MOIRE_INTERFERENCE
    };

    TransformEffectType& operator[](int i) { return order[i]; }
    const TransformEffectType& operator[](int i) const { return order[i]; }
};

// Move effect to end of order array (for newly enabled effects)
inline void MoveTransformToEnd(TransformOrderConfig* config, TransformEffectType type) {
    int idx = -1;
    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
        if (config->order[i] == type) {
            idx = i;
            break;
        }
    }
    if (idx < 0 || idx == TRANSFORM_EFFECT_COUNT - 1) { return; }
    for (int i = idx; i < TRANSFORM_EFFECT_COUNT - 1; i++) {
        config->order[i] = config->order[i + 1];
    }
    config->order[TRANSFORM_EFFECT_COUNT - 1] = type;
}

struct FlowFieldConfig {
    float zoomBase = 0.995f;
    float zoomRadial = 0.0f;
    float rotationSpeed = 0.0f;
    float rotationSpeedRadial = 0.0f;
    float dxBase = 0.0f;
    float dxRadial = 0.0f;
    float dyBase = 0.0f;
    float dyRadial = 0.0f;

    // Center pivot (MilkDrop cx/cy)
    float cx = 0.5f;
    float cy = 0.5f;

    // Directional stretch (MilkDrop sx/sy)
    float sx = 1.0f;
    float sy = 1.0f;

    // Angular modulation
    float zoomAngular = 0.0f;
    int   zoomAngularFreq = 2;
    float rotAngular = 0.0f;
    int   rotAngularFreq = 2;
    float dxAngular = 0.0f;
    int   dxAngularFreq = 2;
    float dyAngular = 0.0f;
    int   dyAngularFreq = 2;
};

struct EffectConfig {
    float halfLife = 0.5f;           // Trail persistence (seconds)
    float blurScale = 1.0f;          // Blur sampling distance (pixels)
    float chromaticOffset = 0.0f;    // RGB channel offset (pixels, 0 = disabled)
    float feedbackDesaturate = 0.05f;// Fade toward dark gray per frame (0.0-0.2)
    FlowFieldConfig flowField;       // Spatial UV flow field parameters
    FeedbackFlowConfig feedbackFlow; // Luminance gradient displacement
    ProceduralWarpConfig proceduralWarp; // MilkDrop animated warp distortion
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

    // Boids flocking simulation
    BoidsConfig boids;

    // Curl advection field simulation
    CurlAdvectionConfig curlAdvection;

    // Cymatics (interference patterns from virtual speakers)
    CymaticsConfig cymatics;

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

    // Watercolor (edge darkening, paper granulation, color bleeding)
    WatercolorConfig watercolor;

    // Neon Glow (Sobel edge detection with colored glow)
    NeonGlowConfig neonGlow;

    // Radial Pulse (polar sine distortion with rings and petals)
    RadialPulseConfig radialPulse;

    // False Color (luminance-based gradient mapping via 1D LUT)
    FalseColorConfig falseColor;

    // Halftone (CMYK dot-matrix print simulation)
    HalftoneConfig halftone;

    // Chladni Warp (Chladni plate nodal line displacement)
    ChladniWarpConfig chladniWarp;

    // Cross-Hatching (NPR procedural diagonal strokes)
    CrossHatchingConfig crossHatching;

    // Palette Quantization (Bayer-dithered color reduction)
    PaletteQuantizationConfig paletteQuantization;

    // Bokeh (golden-angle disc blur with brightness weighting)
    BokehConfig bokeh;

    // Bloom (dual Kawase blur with soft threshold)
    BloomConfig bloom;

    // Mandelbox (box fold + sphere fold fractal transform)
    MandelboxConfig mandelbox;

    // Triangle Fold (Sierpinski-style 3-fold/6-fold gasket patterns)
    TriangleFoldConfig triangleFold;

    // Domain Warp (fbm-based UV displacement with animated drift)
    DomainWarpConfig domainWarp;

    // Phyllotaxis (sunflower seed spiral cellular transform)
    PhyllotaxisConfig phyllotaxis;

    // Phyllotaxis Warp (analytical spiral arm UV displacement)
    PhyllotaxisWarpConfig phyllotaxisWarp;

    // Density Wave Spiral (Lin-Shu density wave theory UV warp)
    DensityWaveSpiralConfig densityWaveSpiral;

    // Moiré Interference (multi-sample UV transform with blended overlays)
    MoireInterferenceConfig moireInterference;

    // Transform effect execution order
    TransformOrderConfig transformOrder;
};

inline bool IsTransformEnabled(const EffectConfig* e, TransformEffectType type) {
    switch (type) {
        case TRANSFORM_SINE_WARP:           return e->sineWarp.enabled;
        case TRANSFORM_KALEIDOSCOPE:        return e->kaleidoscope.enabled;
        case TRANSFORM_INFINITE_ZOOM:       return e->infiniteZoom.enabled;
        case TRANSFORM_RADIAL_STREAK:       return e->radialStreak.enabled;
        case TRANSFORM_TEXTURE_WARP:        return e->textureWarp.enabled;
        case TRANSFORM_VORONOI:             return e->voronoi.enabled;
        case TRANSFORM_WAVE_RIPPLE:         return e->waveRipple.enabled;
        case TRANSFORM_MOBIUS:              return e->mobius.enabled;
        case TRANSFORM_PIXELATION:          return e->pixelation.enabled;
        case TRANSFORM_GLITCH:              return e->glitch.enabled;
        case TRANSFORM_POINCARE_DISK:       return e->poincareDisk.enabled;
        case TRANSFORM_TOON:                return e->toon.enabled;
        case TRANSFORM_HEIGHTFIELD_RELIEF:  return e->heightfieldRelief.enabled;
        case TRANSFORM_GRADIENT_FLOW:       return e->gradientFlow.enabled;
        case TRANSFORM_DROSTE_ZOOM:         return e->drosteZoom.enabled;
        case TRANSFORM_KIFS:                return e->kifs.enabled;
        case TRANSFORM_LATTICE_FOLD:        return e->latticeFold.enabled;
        case TRANSFORM_COLOR_GRADE:         return e->colorGrade.enabled;
        case TRANSFORM_ASCII_ART:           return e->asciiArt.enabled;
        case TRANSFORM_OIL_PAINT:           return e->oilPaint.enabled;
        case TRANSFORM_WATERCOLOR:          return e->watercolor.enabled;
        case TRANSFORM_NEON_GLOW:           return e->neonGlow.enabled;
        case TRANSFORM_RADIAL_PULSE:        return e->radialPulse.enabled;
        case TRANSFORM_FALSE_COLOR:             return e->falseColor.enabled;
        case TRANSFORM_HALFTONE:            return e->halftone.enabled;
        case TRANSFORM_CHLADNI_WARP:        return e->chladniWarp.enabled;
        case TRANSFORM_CROSS_HATCHING:      return e->crossHatching.enabled;
        case TRANSFORM_PALETTE_QUANTIZATION: return e->paletteQuantization.enabled;
        case TRANSFORM_BOKEH:               return e->bokeh.enabled;
        case TRANSFORM_BLOOM:               return e->bloom.enabled;
        case TRANSFORM_MANDELBOX:           return e->mandelbox.enabled;
        case TRANSFORM_TRIANGLE_FOLD:       return e->triangleFold.enabled;
        case TRANSFORM_DOMAIN_WARP:         return e->domainWarp.enabled;
        case TRANSFORM_PHYLLOTAXIS:         return e->phyllotaxis.enabled;
        case TRANSFORM_PHYLLOTAXIS_WARP:    return e->phyllotaxisWarp.enabled;
        case TRANSFORM_PHYSARUM_BOOST:      return e->physarum.enabled && e->physarum.boostIntensity > 0.0f;
        case TRANSFORM_CURL_FLOW_BOOST:     return e->curlFlow.enabled && e->curlFlow.boostIntensity > 0.0f;
        case TRANSFORM_CURL_ADVECTION_BOOST: return e->curlAdvection.enabled && e->curlAdvection.boostIntensity > 0.0f;
        case TRANSFORM_ATTRACTOR_FLOW_BOOST: return e->attractorFlow.enabled && e->attractorFlow.boostIntensity > 0.0f;
        case TRANSFORM_BOIDS_BOOST:         return e->boids.enabled && e->boids.boostIntensity > 0.0f;
        case TRANSFORM_CYMATICS_BOOST:      return e->cymatics.enabled && e->cymatics.boostIntensity > 0.0f;
        case TRANSFORM_DENSITY_WAVE_SPIRAL: return e->densityWaveSpiral.enabled;
        case TRANSFORM_MOIRE_INTERFERENCE: return e->moireInterference.enabled;
        default:                            return false;
    }
}

#endif // EFFECT_CONFIG_H
