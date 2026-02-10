#ifndef EFFECT_DESCRIPTOR_H
#define EFFECT_DESCRIPTOR_H

#include "effect_config.h"
#include <stddef.h>
#include <stdint.h>

// Flag bitmask for effect routing and capabilities
#define EFFECT_FLAG_NONE 0
#define EFFECT_FLAG_BLEND 1
#define EFFECT_FLAG_HALF_RES 2
#define EFFECT_FLAG_SIM_BOOST 4
#define EFFECT_FLAG_NEEDS_RESIZE 8

struct EffectDescriptor {
  TransformEffectType type;
  const char *name;
  const char *categoryBadge;
  int categorySectionIndex;
  size_t enabledOffset;
  uint8_t flags;
};

// clang-format off
static constexpr EffectDescriptor EFFECT_DESCRIPTORS[TRANSFORM_EFFECT_COUNT] = {
    [TRANSFORM_SINE_WARP] = {
        TRANSFORM_SINE_WARP, "Sine Warp", "WARP", 1,
        offsetof(EffectConfig, sineWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_KALEIDOSCOPE] = {
        TRANSFORM_KALEIDOSCOPE, "Kaleidoscope", "SYM", 0,
        offsetof(EffectConfig, kaleidoscope.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_INFINITE_ZOOM] = {
        TRANSFORM_INFINITE_ZOOM, "Infinite Zoom", "MOT", 3,
        offsetof(EffectConfig, infiniteZoom.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_RADIAL_STREAK] = {
        TRANSFORM_RADIAL_STREAK, "Radial Blur", "MOT", 3,
        offsetof(EffectConfig, radialStreak.enabled), EFFECT_FLAG_HALF_RES
    },
    [TRANSFORM_TEXTURE_WARP] = {
        TRANSFORM_TEXTURE_WARP, "Texture Warp", "WARP", 1,
        offsetof(EffectConfig, textureWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_VORONOI] = {
        TRANSFORM_VORONOI, "Voronoi", "CELL", 2,
        offsetof(EffectConfig, voronoi.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_WAVE_RIPPLE] = {
        TRANSFORM_WAVE_RIPPLE, "Wave Ripple", "WARP", 1,
        offsetof(EffectConfig, waveRipple.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_MOBIUS] = {
        TRANSFORM_MOBIUS, "Mobius", "WARP", 1,
        offsetof(EffectConfig, mobius.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_PIXELATION] = {
        TRANSFORM_PIXELATION, "Pixelation", "RET", 6,
        offsetof(EffectConfig, pixelation.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_GLITCH] = {
        TRANSFORM_GLITCH, "Glitch", "RET", 6,
        offsetof(EffectConfig, glitch.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_POINCARE_DISK] = {
        TRANSFORM_POINCARE_DISK, "Poincare Disk", "SYM", 0,
        offsetof(EffectConfig, poincareDisk.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_TOON] = {
        TRANSFORM_TOON, "Toon", "GFX", 5,
        offsetof(EffectConfig, toon.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_HEIGHTFIELD_RELIEF] = {
        TRANSFORM_HEIGHTFIELD_RELIEF, "Heightfield Relief", "OPT", 7,
        offsetof(EffectConfig, heightfieldRelief.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_GRADIENT_FLOW] = {
        TRANSFORM_GRADIENT_FLOW, "Gradient Flow", "WARP", 1,
        offsetof(EffectConfig, gradientFlow.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_DROSTE_ZOOM] = {
        TRANSFORM_DROSTE_ZOOM, "Droste Zoom", "MOT", 3,
        offsetof(EffectConfig, drosteZoom.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_KIFS] = {
        TRANSFORM_KIFS, "KIFS", "SYM", 0,
        offsetof(EffectConfig, kifs.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_LATTICE_FOLD] = {
        TRANSFORM_LATTICE_FOLD, "Lattice Fold", "CELL", 2,
        offsetof(EffectConfig, latticeFold.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_COLOR_GRADE] = {
        TRANSFORM_COLOR_GRADE, "Color Grade", "COL", 8,
        offsetof(EffectConfig, colorGrade.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_ASCII_ART] = {
        TRANSFORM_ASCII_ART, "ASCII Art", "RET", 6,
        offsetof(EffectConfig, asciiArt.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_OIL_PAINT] = {
        TRANSFORM_OIL_PAINT, "Oil Paint", "ART", 4,
        offsetof(EffectConfig, oilPaint.enabled), EFFECT_FLAG_NEEDS_RESIZE
    },
    [TRANSFORM_WATERCOLOR] = {
        TRANSFORM_WATERCOLOR, "Watercolor", "ART", 4,
        offsetof(EffectConfig, watercolor.enabled), EFFECT_FLAG_HALF_RES
    },
    [TRANSFORM_NEON_GLOW] = {
        TRANSFORM_NEON_GLOW, "Neon Glow", "GFX", 5,
        offsetof(EffectConfig, neonGlow.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_RADIAL_PULSE] = {
        TRANSFORM_RADIAL_PULSE, "Radial Pulse", "WARP", 1,
        offsetof(EffectConfig, radialPulse.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_FALSE_COLOR] = {
        TRANSFORM_FALSE_COLOR, "False Color", "COL", 8,
        offsetof(EffectConfig, falseColor.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_HALFTONE] = {
        TRANSFORM_HALFTONE, "Halftone", "GFX", 5,
        offsetof(EffectConfig, halftone.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_CHLADNI_WARP] = {
        TRANSFORM_CHLADNI_WARP, "Chladni Warp", "WARP", 1,
        offsetof(EffectConfig, chladniWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_CROSS_HATCHING] = {
        TRANSFORM_CROSS_HATCHING, "Cross-Hatching", "ART", 4,
        offsetof(EffectConfig, crossHatching.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_PALETTE_QUANTIZATION] = {
        TRANSFORM_PALETTE_QUANTIZATION, "Palette Quantization", "COL", 8,
        offsetof(EffectConfig, paletteQuantization.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_BOKEH] = {
        TRANSFORM_BOKEH, "Bokeh", "OPT", 7,
        offsetof(EffectConfig, bokeh.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_BLOOM] = {
        TRANSFORM_BLOOM, "Bloom", "OPT", 7,
        offsetof(EffectConfig, bloom.enabled), EFFECT_FLAG_NEEDS_RESIZE
    },
    [TRANSFORM_MANDELBOX] = {
        TRANSFORM_MANDELBOX, "Mandelbox", "SYM", 0,
        offsetof(EffectConfig, mandelbox.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_TRIANGLE_FOLD] = {
        TRANSFORM_TRIANGLE_FOLD, "Triangle Fold", "SYM", 0,
        offsetof(EffectConfig, triangleFold.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_DOMAIN_WARP] = {
        TRANSFORM_DOMAIN_WARP, "Domain Warp", "WARP", 1,
        offsetof(EffectConfig, domainWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_PHYLLOTAXIS] = {
        TRANSFORM_PHYLLOTAXIS, "Phyllotaxis", "CELL", 2,
        offsetof(EffectConfig, phyllotaxis.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_PHYSARUM_BOOST] = {
        TRANSFORM_PHYSARUM_BOOST, "Physarum Boost", "SIM", 9,
        offsetof(EffectConfig, physarum.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_CURL_FLOW_BOOST] = {
        TRANSFORM_CURL_FLOW_BOOST, "Curl Flow Boost", "SIM", 9,
        offsetof(EffectConfig, curlFlow.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_CURL_ADVECTION_BOOST] = {
        TRANSFORM_CURL_ADVECTION_BOOST, "Curl Advection Boost", "SIM", 9,
        offsetof(EffectConfig, curlAdvection.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_ATTRACTOR_FLOW_BOOST] = {
        TRANSFORM_ATTRACTOR_FLOW_BOOST, "Attractor Flow Boost", "SIM", 9,
        offsetof(EffectConfig, attractorFlow.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_BOIDS_BOOST] = {
        TRANSFORM_BOIDS_BOOST, "Boids Boost", "SIM", 9,
        offsetof(EffectConfig, boids.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_CYMATICS_BOOST] = {
        TRANSFORM_CYMATICS_BOOST, "Cymatics Boost", "SIM", 9,
        offsetof(EffectConfig, cymatics.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_PARTICLE_LIFE_BOOST] = {
        TRANSFORM_PARTICLE_LIFE_BOOST, "Particle Life Boost", "SIM", 9,
        offsetof(EffectConfig, particleLife.enabled), EFFECT_FLAG_SIM_BOOST
    },
    [TRANSFORM_DENSITY_WAVE_SPIRAL] = {
        TRANSFORM_DENSITY_WAVE_SPIRAL, "Density Wave Spiral", "MOT", 3,
        offsetof(EffectConfig, densityWaveSpiral.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_MOIRE_INTERFERENCE] = {
        TRANSFORM_MOIRE_INTERFERENCE, "Moire Interference", "SYM", 0,
        offsetof(EffectConfig, moireInterference.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_PENCIL_SKETCH] = {
        TRANSFORM_PENCIL_SKETCH, "Pencil Sketch", "ART", 4,
        offsetof(EffectConfig, pencilSketch.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_MATRIX_RAIN] = {
        TRANSFORM_MATRIX_RAIN, "Matrix Rain", "RET", 6,
        offsetof(EffectConfig, matrixRain.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_IMPRESSIONIST] = {
        TRANSFORM_IMPRESSIONIST, "Impressionist", "ART", 4,
        offsetof(EffectConfig, impressionist.enabled), EFFECT_FLAG_HALF_RES
    },
    [TRANSFORM_KUWAHARA] = {
        TRANSFORM_KUWAHARA, "Kuwahara", "GFX", 5,
        offsetof(EffectConfig, kuwahara.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_INK_WASH] = {
        TRANSFORM_INK_WASH, "Ink Wash", "ART", 4,
        offsetof(EffectConfig, inkWash.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_DISCO_BALL] = {
        TRANSFORM_DISCO_BALL, "Disco Ball", "GFX", 5,
        offsetof(EffectConfig, discoBall.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_SURFACE_WARP] = {
        TRANSFORM_SURFACE_WARP, "Surface Warp", "WARP", 1,
        offsetof(EffectConfig, surfaceWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_INTERFERENCE_WARP] = {
        TRANSFORM_INTERFERENCE_WARP, "Interference Warp", "WARP", 1,
        offsetof(EffectConfig, interferenceWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_CORRIDOR_WARP] = {
        TRANSFORM_CORRIDOR_WARP, "Corridor Warp", "WARP", 1,
        offsetof(EffectConfig, corridorWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_SHAKE] = {
        TRANSFORM_SHAKE, "Shake", "MOT", 3,
        offsetof(EffectConfig, shake.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_LEGO_BRICKS] = {
        TRANSFORM_LEGO_BRICKS, "LEGO Bricks", "GFX", 5,
        offsetof(EffectConfig, legoBricks.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_RADIAL_IFS] = {
        TRANSFORM_RADIAL_IFS, "Radial IFS", "SYM", 0,
        offsetof(EffectConfig, radialIfs.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_CIRCUIT_BOARD] = {
        TRANSFORM_CIRCUIT_BOARD, "Circuit Board", "WARP", 1,
        offsetof(EffectConfig, circuitBoard.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_SYNTHWAVE] = {
        TRANSFORM_SYNTHWAVE, "Synthwave", "RET", 6,
        offsetof(EffectConfig, synthwave.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_RELATIVISTIC_DOPPLER] = {
        TRANSFORM_RELATIVISTIC_DOPPLER, "Relativistic Doppler", "MOT", 3,
        offsetof(EffectConfig, relativisticDoppler.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_ANAMORPHIC_STREAK] = {
        TRANSFORM_ANAMORPHIC_STREAK, "Anamorphic Streak", "OPT", 7,
        offsetof(EffectConfig, anamorphicStreak.enabled), EFFECT_FLAG_NEEDS_RESIZE
    },
    [TRANSFORM_FFT_RADIAL_WARP] = {
        TRANSFORM_FFT_RADIAL_WARP, "FFT Radial Warp", "WARP", 1,
        offsetof(EffectConfig, fftRadialWarp.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_CONSTELLATION_BLEND] = {
        TRANSFORM_CONSTELLATION_BLEND, "Constellation Blend", "GEN", 10,
        offsetof(EffectConfig, constellation.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_PLASMA_BLEND] = {
        TRANSFORM_PLASMA_BLEND, "Plasma Blend", "GEN", 10,
        offsetof(EffectConfig, plasma.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_INTERFERENCE_BLEND] = {
        TRANSFORM_INTERFERENCE_BLEND, "Interference Blend", "GEN", 10,
        offsetof(EffectConfig, interference.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_SOLID_COLOR] = {
        TRANSFORM_SOLID_COLOR, "Solid Color", "GEN", 10,
        offsetof(EffectConfig, solidColor.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_SCAN_BARS_BLEND] = {
        TRANSFORM_SCAN_BARS_BLEND, "Scan Bars Blend", "GEN", 10,
        offsetof(EffectConfig, scanBars.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_PITCH_SPIRAL_BLEND] = {
        TRANSFORM_PITCH_SPIRAL_BLEND, "Pitch Spiral Blend", "GEN", 10,
        offsetof(EffectConfig, pitchSpiral.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_MULTI_SCALE_GRID] = {
        TRANSFORM_MULTI_SCALE_GRID, "Multi-Scale Grid", "CELL", 2,
        offsetof(EffectConfig, multiScaleGrid.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_MOIRE_GENERATOR_BLEND] = {
        TRANSFORM_MOIRE_GENERATOR_BLEND, "Moire Generator Blend", "GEN", 10,
        offsetof(EffectConfig, moireGenerator.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_SPECTRAL_ARCS_BLEND] = {
        TRANSFORM_SPECTRAL_ARCS_BLEND, "Spectral Arcs Blend", "GEN", 10,
        offsetof(EffectConfig, spectralArcs.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_MUONS_BLEND] = {
        TRANSFORM_MUONS_BLEND, "Muons Blend", "GEN", 10,
        offsetof(EffectConfig, muons.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_FILAMENTS_BLEND] = {
        TRANSFORM_FILAMENTS_BLEND, "Filaments Blend", "GEN", 10,
        offsetof(EffectConfig, filaments.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_SLASHES_BLEND] = {
        TRANSFORM_SLASHES_BLEND, "Slashes Blend", "GEN", 10,
        offsetof(EffectConfig, slashes.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_GLYPH_FIELD_BLEND] = {
        TRANSFORM_GLYPH_FIELD_BLEND, "Glyph Field Blend", "GEN", 10,
        offsetof(EffectConfig, glyphField.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_ARC_STROBE_BLEND] = {
        TRANSFORM_ARC_STROBE_BLEND, "Arc Strobe Blend", "GEN", 10,
        offsetof(EffectConfig, arcStrobe.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_SIGNAL_FRAMES_BLEND] = {
        TRANSFORM_SIGNAL_FRAMES_BLEND, "Signal Frames Blend", "GEN", 10,
        offsetof(EffectConfig, signalFrames.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_NEBULA_BLEND] = {
        TRANSFORM_NEBULA_BLEND, "Nebula Blend", "GEN", 10,
        offsetof(EffectConfig, nebula.enabled), EFFECT_FLAG_BLEND
    },
    [TRANSFORM_CRT] = {
        TRANSFORM_CRT, "CRT", "RET", 6,
        offsetof(EffectConfig, crt.enabled), EFFECT_FLAG_NONE
    },
    [TRANSFORM_DOT_MATRIX] = {
        TRANSFORM_DOT_MATRIX, "Dot Matrix", "CELL", 2,
        offsetof(EffectConfig, dotMatrix.enabled), EFFECT_FLAG_NONE
    },
};
// clang-format on

// Category badge and section index pair
struct EffectCategory {
  const char *badge;
  int sectionIndex;
};

// Returns display name for the given effect type
const char *EffectDescriptorName(TransformEffectType type);

// Returns category badge string and section color index
EffectCategory EffectDescriptorCategory(TransformEffectType type);

// Reads the enabled bool at the descriptor's enabledOffset within EffectConfig
bool IsDescriptorEnabled(const EffectConfig *e, TransformEffectType type);

// Convenience wrappers preserving the original API names
inline const char *TransformEffectName(TransformEffectType type) {
  return EffectDescriptorName(type);
}

inline bool IsTransformEnabled(const EffectConfig *e,
                               TransformEffectType type) {
  return IsDescriptorEnabled(e, type);
}

#endif // EFFECT_DESCRIPTOR_H
