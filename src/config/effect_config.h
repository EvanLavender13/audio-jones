#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "effects/anamorphic_streak.h"
#include "effects/arc_strobe.h"
#include "effects/ascii_art.h"
#include "effects/bloom.h"
#include "effects/bokeh.h"
#include "effects/chladni_warp.h"
#include "effects/circuit_board.h"
#include "effects/color_grade.h"
#include "effects/constellation.h"
#include "effects/corridor_warp.h"
#include "effects/cross_hatching.h"
#include "effects/crt.h"
#include "effects/density_wave_spiral.h"
#include "effects/disco_ball.h"
#include "effects/domain_warp.h"
#include "effects/dot_matrix.h"
#include "effects/droste_zoom.h"
#include "effects/false_color.h"
#include "effects/fft_radial_warp.h"
#include "effects/filaments.h"
#include "effects/glitch.h"
#include "effects/glyph_field.h"
#include "effects/gradient_flow.h"
#include "effects/halftone.h"
#include "effects/heightfield_relief.h"
#include "effects/impressionist.h"
#include "effects/infinite_zoom.h"
#include "effects/ink_wash.h"
#include "effects/interference.h"
#include "effects/interference_warp.h"
#include "effects/kaleidoscope.h"
#include "effects/kifs.h"
#include "effects/kuwahara.h"
#include "effects/lattice_fold.h"
#include "effects/lego_bricks.h"
#include "effects/mandelbox.h"
#include "effects/matrix_rain.h"
#include "effects/mobius.h"
#include "effects/moire_generator.h"
#include "effects/moire_interference.h"
#include "effects/multi_scale_grid.h"
#include "effects/muons.h"
#include "effects/neon_glow.h"
#include "effects/oil_paint.h"
#include "effects/palette_quantization.h"
#include "effects/pencil_sketch.h"
#include "effects/phyllotaxis.h"
#include "effects/pitch_spiral.h"
#include "effects/pixelation.h"
#include "effects/plasma.h"
#include "effects/poincare_disk.h"
#include "effects/radial_ifs.h"
#include "effects/radial_pulse.h"
#include "effects/radial_streak.h"
#include "effects/relativistic_doppler.h"
#include "effects/scan_bars.h"
#include "effects/shake.h"
#include "effects/signal_frames.h"
#include "effects/sine_warp.h"
#include "effects/slashes.h"
#include "effects/solid_color.h"
#include "effects/spectral_arcs.h"
#include "effects/surface_warp.h"
#include "effects/synthwave.h"
#include "effects/texture_warp.h"
#include "effects/toon.h"
#include "effects/triangle_fold.h"
#include "effects/voronoi.h"
#include "effects/watercolor.h"
#include "effects/wave_ripple.h"
#include "feedback_flow_config.h"
#include "procedural_warp_config.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_advection.h"
#include "simulation/curl_flow.h"
#include "simulation/cymatics.h"
#include "simulation/particle_life.h"
#include "simulation/physarum.h"

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
  TRANSFORM_PHYSARUM_BOOST,
  TRANSFORM_CURL_FLOW_BOOST,
  TRANSFORM_CURL_ADVECTION_BOOST,
  TRANSFORM_ATTRACTOR_FLOW_BOOST,
  TRANSFORM_BOIDS_BOOST,
  TRANSFORM_CYMATICS_BOOST,
  TRANSFORM_PARTICLE_LIFE_BOOST,
  TRANSFORM_DENSITY_WAVE_SPIRAL,
  TRANSFORM_MOIRE_INTERFERENCE,
  TRANSFORM_PENCIL_SKETCH,
  TRANSFORM_MATRIX_RAIN,
  TRANSFORM_IMPRESSIONIST,
  TRANSFORM_KUWAHARA,
  TRANSFORM_INK_WASH,
  TRANSFORM_DISCO_BALL,
  TRANSFORM_SURFACE_WARP,
  TRANSFORM_INTERFERENCE_WARP,
  TRANSFORM_CORRIDOR_WARP,
  TRANSFORM_SHAKE,
  TRANSFORM_LEGO_BRICKS,
  TRANSFORM_RADIAL_IFS,
  TRANSFORM_CIRCUIT_BOARD,
  TRANSFORM_SYNTHWAVE,
  TRANSFORM_RELATIVISTIC_DOPPLER,
  TRANSFORM_ANAMORPHIC_STREAK,
  TRANSFORM_FFT_RADIAL_WARP,
  TRANSFORM_CONSTELLATION_BLEND,
  TRANSFORM_PLASMA_BLEND,
  TRANSFORM_INTERFERENCE_BLEND,
  TRANSFORM_SOLID_COLOR,
  TRANSFORM_SCAN_BARS_BLEND,
  TRANSFORM_PITCH_SPIRAL_BLEND,
  TRANSFORM_MULTI_SCALE_GRID,
  TRANSFORM_MOIRE_GENERATOR_BLEND,
  TRANSFORM_SPECTRAL_ARCS_BLEND,
  TRANSFORM_MUONS_BLEND,
  TRANSFORM_FILAMENTS_BLEND,
  TRANSFORM_SLASHES_BLEND,
  TRANSFORM_GLYPH_FIELD_BLEND,
  TRANSFORM_ARC_STROBE_BLEND,
  TRANSFORM_SIGNAL_FRAMES_BLEND,
  TRANSFORM_CRT,
  TRANSFORM_DOT_MATRIX,
  TRANSFORM_EFFECT_COUNT
};

constexpr const char *TRANSFORM_EFFECT_NAMES[TRANSFORM_EFFECT_COUNT] = {
    "Sine Warp",             // TRANSFORM_SINE_WARP
    "Kaleidoscope",          // TRANSFORM_KALEIDOSCOPE
    "Infinite Zoom",         // TRANSFORM_INFINITE_ZOOM
    "Radial Blur",           // TRANSFORM_RADIAL_STREAK
    "Texture Warp",          // TRANSFORM_TEXTURE_WARP
    "Voronoi",               // TRANSFORM_VORONOI
    "Wave Ripple",           // TRANSFORM_WAVE_RIPPLE
    "Mobius",                // TRANSFORM_MOBIUS
    "Pixelation",            // TRANSFORM_PIXELATION
    "Glitch",                // TRANSFORM_GLITCH
    "Poincare Disk",         // TRANSFORM_POINCARE_DISK
    "Toon",                  // TRANSFORM_TOON
    "Heightfield Relief",    // TRANSFORM_HEIGHTFIELD_RELIEF
    "Gradient Flow",         // TRANSFORM_GRADIENT_FLOW
    "Droste Zoom",           // TRANSFORM_DROSTE_ZOOM
    "KIFS",                  // TRANSFORM_KIFS
    "Lattice Fold",          // TRANSFORM_LATTICE_FOLD
    "Color Grade",           // TRANSFORM_COLOR_GRADE
    "ASCII Art",             // TRANSFORM_ASCII_ART
    "Oil Paint",             // TRANSFORM_OIL_PAINT
    "Watercolor",            // TRANSFORM_WATERCOLOR
    "Neon Glow",             // TRANSFORM_NEON_GLOW
    "Radial Pulse",          // TRANSFORM_RADIAL_PULSE
    "False Color",           // TRANSFORM_FALSE_COLOR
    "Halftone",              // TRANSFORM_HALFTONE
    "Chladni Warp",          // TRANSFORM_CHLADNI_WARP
    "Cross-Hatching",        // TRANSFORM_CROSS_HATCHING
    "Palette Quantization",  // TRANSFORM_PALETTE_QUANTIZATION
    "Bokeh",                 // TRANSFORM_BOKEH
    "Bloom",                 // TRANSFORM_BLOOM
    "Mandelbox",             // TRANSFORM_MANDELBOX
    "Triangle Fold",         // TRANSFORM_TRIANGLE_FOLD
    "Domain Warp",           // TRANSFORM_DOMAIN_WARP
    "Phyllotaxis",           // TRANSFORM_PHYLLOTAXIS
    "Physarum Boost",        // TRANSFORM_PHYSARUM_BOOST
    "Curl Flow Boost",       // TRANSFORM_CURL_FLOW_BOOST
    "Curl Advection Boost",  // TRANSFORM_CURL_ADVECTION_BOOST
    "Attractor Flow Boost",  // TRANSFORM_ATTRACTOR_FLOW_BOOST
    "Boids Boost",           // TRANSFORM_BOIDS_BOOST
    "Cymatics Boost",        // TRANSFORM_CYMATICS_BOOST
    "Particle Life Boost",   // TRANSFORM_PARTICLE_LIFE_BOOST
    "Density Wave Spiral",   // TRANSFORM_DENSITY_WAVE_SPIRAL
    "Moire Interference",    // TRANSFORM_MOIRE_INTERFERENCE
    "Pencil Sketch",         // TRANSFORM_PENCIL_SKETCH
    "Matrix Rain",           // TRANSFORM_MATRIX_RAIN
    "Impressionist",         // TRANSFORM_IMPRESSIONIST
    "Kuwahara",              // TRANSFORM_KUWAHARA
    "Ink Wash",              // TRANSFORM_INK_WASH
    "Disco Ball",            // TRANSFORM_DISCO_BALL
    "Surface Warp",          // TRANSFORM_SURFACE_WARP
    "Interference Warp",     // TRANSFORM_INTERFERENCE_WARP
    "Corridor Warp",         // TRANSFORM_CORRIDOR_WARP
    "Shake",                 // TRANSFORM_SHAKE
    "LEGO Bricks",           // TRANSFORM_LEGO_BRICKS
    "Radial IFS",            // TRANSFORM_RADIAL_IFS
    "Circuit Board",         // TRANSFORM_CIRCUIT_BOARD
    "Synthwave",             // TRANSFORM_SYNTHWAVE
    "Relativistic Doppler",  // TRANSFORM_RELATIVISTIC_DOPPLER
    "Anamorphic Streak",     // TRANSFORM_ANAMORPHIC_STREAK
    "FFT Radial Warp",       // TRANSFORM_FFT_RADIAL_WARP
    "Constellation Blend",   // TRANSFORM_CONSTELLATION_BLEND
    "Plasma Blend",          // TRANSFORM_PLASMA_BLEND
    "Interference Blend",    // TRANSFORM_INTERFERENCE_BLEND
    "Solid Color",           // TRANSFORM_SOLID_COLOR
    "Scan Bars Blend",       // TRANSFORM_SCAN_BARS_BLEND
    "Pitch Spiral Blend",    // TRANSFORM_PITCH_SPIRAL_BLEND
    "Multi-Scale Grid",      // TRANSFORM_MULTI_SCALE_GRID
    "Moire Generator Blend", // TRANSFORM_MOIRE_GENERATOR_BLEND
    "Spectral Arcs Blend",   // TRANSFORM_SPECTRAL_ARCS_BLEND
    "Muons Blend",           // TRANSFORM_MUONS_BLEND
    "Filaments Blend",       // TRANSFORM_FILAMENTS_BLEND
    "Slashes Blend",         // TRANSFORM_SLASHES_BLEND
    "Glyph Field Blend",     // TRANSFORM_GLYPH_FIELD_BLEND
    "Arc Strobe Blend",      // TRANSFORM_ARC_STROBE_BLEND
    "Signal Frames Blend",   // TRANSFORM_SIGNAL_FRAMES_BLEND
    "CRT",                   // TRANSFORM_CRT
    "Dot Matrix",            // TRANSFORM_DOT_MATRIX
};

inline const char *TransformEffectName(TransformEffectType type) {
  if (type >= 0 && type < TRANSFORM_EFFECT_COUNT) {
    return TRANSFORM_EFFECT_NAMES[type];
  }
  return "Unknown";
}

// Forward declaration for IsTransformEnabled
struct EffectConfig;

// Check if a transform effect is enabled in the config
inline bool IsTransformEnabled(const EffectConfig *e, TransformEffectType type);

struct TransformOrderConfig {
  TransformEffectType order[TRANSFORM_EFFECT_COUNT];

  TransformOrderConfig() {
    for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
      order[i] = (TransformEffectType)i;
    }
  }

  TransformEffectType &operator[](int i) { return order[i]; }
  const TransformEffectType &operator[](int i) const { return order[i]; }
};

// Move effect to end of order array (for newly enabled effects)
inline void MoveTransformToEnd(TransformOrderConfig *config,
                               TransformEffectType type) {
  int idx = -1;
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (config->order[i] == type) {
      idx = i;
      break;
    }
  }
  if (idx < 0 || idx == TRANSFORM_EFFECT_COUNT - 1) {
    return;
  }
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
  int zoomAngularFreq = 2;
  float rotAngular = 0.0f;
  int rotAngularFreq = 2;
  float dxAngular = 0.0f;
  int dxAngularFreq = 2;
  float dyAngular = 0.0f;
  int dyAngularFreq = 2;
};

struct EffectConfig {
  float halfLife = 0.5f;            // Trail persistence (seconds)
  float blurScale = 1.0f;           // Blur sampling distance (pixels)
  float chromaticOffset = 0.0f;     // RGB channel offset (pixels, 0 = disabled)
  float feedbackDesaturate = 0.05f; // Fade toward dark gray per frame (0.0-0.2)
  float motionScale = 1.0f;  // Global feedback motion time-dilation (0.01-1.0)
  FlowFieldConfig flowField; // Spatial UV flow field parameters
  FeedbackFlowConfig feedbackFlow;     // Luminance gradient displacement
  ProceduralWarpConfig proceduralWarp; // MilkDrop animated warp distortion
  float gamma = 1.0f;   // Display gamma correction (1.0 = disabled)
  float clarity = 0.0f; // Local contrast enhancement (0.0 = disabled)

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

  // Particle Life (emergent multi-species particle simulation)
  ParticleLifeConfig particleLife;

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

  // Plasma (procedural plasma texture generator)
  PlasmaConfig plasma;

  // Glitch (Analog, Digital, VHS video corruption)
  GlitchConfig glitch;

  // CRT (cathode ray tube display simulation)
  CrtConfig crt;

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

  // Corridor Warp (infinite floor/ceiling perspective projection)
  CorridorWarpConfig corridorWarp;

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

  // Circuit Board (fractal grid distortion with chromatic aberration)
  CircuitBoardConfig circuitBoard;

  // Constellation (star field with connecting lines)
  ConstellationConfig constellation;

  // Interference (multi-source wave superposition generator)
  InterferenceConfig interference;

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

  // Radial IFS (iterated polar wedge folding for snowflake/flower fractals)
  RadialIfsConfig radialIfs;

  // Domain Warp (fbm-based UV displacement with animated drift)
  DomainWarpConfig domainWarp;

  // Phyllotaxis (sunflower seed spiral cellular transform)
  PhyllotaxisConfig phyllotaxis;

  // Density Wave Spiral (Lin-Shu density wave theory UV warp)
  DensityWaveSpiralConfig densityWaveSpiral;

  // Moiré Interference (multi-sample UV transform with blended overlays)
  MoireInterferenceConfig moireInterference;

  // Pencil Sketch (directional gradient-aligned stroke accumulation)
  PencilSketchConfig pencilSketch;

  // Matrix Rain (falling procedural rune columns)
  MatrixRainConfig matrixRain;

  // Impressionist (overlapping circular brush dabs with hatching and paper
  // grain)
  ImpressionistConfig impressionist;

  // Kuwahara (edge-preserving painterly smoothing)
  KuwaharaConfig kuwahara;

  // Ink Wash (Sobel edge darkening with paper grain and color bleed)
  InkWashConfig inkWash;

  // Disco Ball (faceted mirror sphere reflection effect)
  DiscoBallConfig discoBall;

  // Shake (motion blur jitter via multi-sample averaging)
  ShakeConfig shake;

  // Surface Warp (rolling-hills gradient displacement)
  SurfaceWarpConfig surfaceWarp;

  // Interference Warp (multi-axis superposed harmonic distortion)
  InterferenceWarpConfig interferenceWarp;

  // LEGO Bricks (stylized brick-toy aesthetic with studs and shadows)
  LegoBricksConfig legoBricks;

  // Synthwave (80s retrofuturism)
  SynthwaveConfig synthwave;

  // Relativistic Doppler (special relativity light aberration and color shift)
  RelativisticDopplerConfig relativisticDoppler;

  // Anamorphic Streak (horizontal lens flare with chromatic separation)
  AnamorphicStreakConfig anamorphicStreak;

  // FFT Radial Warp (audio-reactive radial displacement)
  FftRadialWarpConfig fftRadialWarp;

  // Solid Color (flat color generator with blend)
  SolidColorConfig solidColor;

  // Scan Bars (scrolling luminance bars with blend)
  ScanBarsConfig scanBars;

  // Pitch Spiral (logarithmic frequency spiral overlay)
  PitchSpiralConfig pitchSpiral;

  // Multi-Scale Grid (nested grid cellular subdivision)
  MultiScaleGridConfig multiScaleGrid;

  // Moire Generator (procedural moire pattern generator with blend)
  MoireGeneratorConfig moireGenerator;

  // Spectral Arcs (frequency-band arc overlay with blend)
  SpectralArcsConfig spectralArcs;

  // Muons (raymarched turbulent ring structures with blend)
  MuonsConfig muons;

  // Filaments (radial semitone burst with triangle-noise displacement)
  FilamentsConfig filaments;

  // Slashes (chaotic per-semitone rectangular bar scatter)
  SlashesConfig slashes;

  // Glyph Field (typographic symbol grid with audio-reactive modulation)
  GlyphFieldConfig glyphField;

  // Arc Strobe (electric arc network generator with blend)
  ArcStrobeConfig arcStrobe;

  // Signal Frames (FFT-driven concentric rounded-rectangle outlines with blend)
  SignalFramesConfig signalFrames;

  // Dot Matrix (circular dot grid with size/color modulation)
  DotMatrixConfig dotMatrix;

  // Transform effect execution order
  TransformOrderConfig transformOrder;
};

inline bool IsTransformEnabled(const EffectConfig *e,
                               TransformEffectType type) {
  switch (type) {
  case TRANSFORM_SINE_WARP:
    return e->sineWarp.enabled;
  case TRANSFORM_KALEIDOSCOPE:
    return e->kaleidoscope.enabled;
  case TRANSFORM_INFINITE_ZOOM:
    return e->infiniteZoom.enabled;
  case TRANSFORM_RADIAL_STREAK:
    return e->radialStreak.enabled;
  case TRANSFORM_TEXTURE_WARP:
    return e->textureWarp.enabled;
  case TRANSFORM_VORONOI:
    return e->voronoi.enabled;
  case TRANSFORM_WAVE_RIPPLE:
    return e->waveRipple.enabled;
  case TRANSFORM_MOBIUS:
    return e->mobius.enabled;
  case TRANSFORM_PIXELATION:
    return e->pixelation.enabled;
  case TRANSFORM_GLITCH:
    return e->glitch.enabled;
  case TRANSFORM_POINCARE_DISK:
    return e->poincareDisk.enabled;
  case TRANSFORM_TOON:
    return e->toon.enabled;
  case TRANSFORM_HEIGHTFIELD_RELIEF:
    return e->heightfieldRelief.enabled;
  case TRANSFORM_GRADIENT_FLOW:
    return e->gradientFlow.enabled;
  case TRANSFORM_DROSTE_ZOOM:
    return e->drosteZoom.enabled;
  case TRANSFORM_KIFS:
    return e->kifs.enabled;
  case TRANSFORM_LATTICE_FOLD:
    return e->latticeFold.enabled;
  case TRANSFORM_COLOR_GRADE:
    return e->colorGrade.enabled;
  case TRANSFORM_ASCII_ART:
    return e->asciiArt.enabled;
  case TRANSFORM_OIL_PAINT:
    return e->oilPaint.enabled;
  case TRANSFORM_WATERCOLOR:
    return e->watercolor.enabled;
  case TRANSFORM_NEON_GLOW:
    return e->neonGlow.enabled;
  case TRANSFORM_RADIAL_PULSE:
    return e->radialPulse.enabled;
  case TRANSFORM_FALSE_COLOR:
    return e->falseColor.enabled;
  case TRANSFORM_HALFTONE:
    return e->halftone.enabled;
  case TRANSFORM_CHLADNI_WARP:
    return e->chladniWarp.enabled;
  case TRANSFORM_CROSS_HATCHING:
    return e->crossHatching.enabled;
  case TRANSFORM_PALETTE_QUANTIZATION:
    return e->paletteQuantization.enabled;
  case TRANSFORM_BOKEH:
    return e->bokeh.enabled;
  case TRANSFORM_BLOOM:
    return e->bloom.enabled;
  case TRANSFORM_MANDELBOX:
    return e->mandelbox.enabled;
  case TRANSFORM_TRIANGLE_FOLD:
    return e->triangleFold.enabled;
  case TRANSFORM_DOMAIN_WARP:
    return e->domainWarp.enabled;
  case TRANSFORM_PHYLLOTAXIS:
    return e->phyllotaxis.enabled;
  case TRANSFORM_PHYSARUM_BOOST:
    return e->physarum.enabled && e->physarum.boostIntensity > 0.0f;
  case TRANSFORM_CURL_FLOW_BOOST:
    return e->curlFlow.enabled && e->curlFlow.boostIntensity > 0.0f;
  case TRANSFORM_CURL_ADVECTION_BOOST:
    return e->curlAdvection.enabled && e->curlAdvection.boostIntensity > 0.0f;
  case TRANSFORM_ATTRACTOR_FLOW_BOOST:
    return e->attractorFlow.enabled && e->attractorFlow.boostIntensity > 0.0f;
  case TRANSFORM_BOIDS_BOOST:
    return e->boids.enabled && e->boids.boostIntensity > 0.0f;
  case TRANSFORM_CYMATICS_BOOST:
    return e->cymatics.enabled && e->cymatics.boostIntensity > 0.0f;
  case TRANSFORM_PARTICLE_LIFE_BOOST:
    return e->particleLife.enabled && e->particleLife.boostIntensity > 0.0f;
  case TRANSFORM_DENSITY_WAVE_SPIRAL:
    return e->densityWaveSpiral.enabled;
  case TRANSFORM_MOIRE_INTERFERENCE:
    return e->moireInterference.enabled;
  case TRANSFORM_PENCIL_SKETCH:
    return e->pencilSketch.enabled;
  case TRANSFORM_MATRIX_RAIN:
    return e->matrixRain.enabled;
  case TRANSFORM_IMPRESSIONIST:
    return e->impressionist.enabled;
  case TRANSFORM_KUWAHARA:
    return e->kuwahara.enabled;
  case TRANSFORM_INK_WASH:
    return e->inkWash.enabled;
  case TRANSFORM_DISCO_BALL:
    return e->discoBall.enabled;
  case TRANSFORM_SURFACE_WARP:
    return e->surfaceWarp.enabled;
  case TRANSFORM_INTERFERENCE_WARP:
    return e->interferenceWarp.enabled;
  case TRANSFORM_CORRIDOR_WARP:
    return e->corridorWarp.enabled;
  case TRANSFORM_SHAKE:
    return e->shake.enabled;
  case TRANSFORM_LEGO_BRICKS:
    return e->legoBricks.enabled;
  case TRANSFORM_RADIAL_IFS:
    return e->radialIfs.enabled;
  case TRANSFORM_CIRCUIT_BOARD:
    return e->circuitBoard.enabled;
  case TRANSFORM_SYNTHWAVE:
    return e->synthwave.enabled;
  case TRANSFORM_RELATIVISTIC_DOPPLER:
    return e->relativisticDoppler.enabled;
  case TRANSFORM_ANAMORPHIC_STREAK:
    return e->anamorphicStreak.enabled;
  case TRANSFORM_FFT_RADIAL_WARP:
    return e->fftRadialWarp.enabled;
  case TRANSFORM_CONSTELLATION_BLEND:
    return e->constellation.enabled && e->constellation.blendIntensity > 0.0f;
  case TRANSFORM_PLASMA_BLEND:
    return e->plasma.enabled && e->plasma.blendIntensity > 0.0f;
  case TRANSFORM_INTERFERENCE_BLEND:
    return e->interference.enabled && e->interference.blendIntensity > 0.0f;
  case TRANSFORM_SOLID_COLOR:
    return e->solidColor.enabled && e->solidColor.blendIntensity > 0.0f;
  case TRANSFORM_SCAN_BARS_BLEND:
    return e->scanBars.enabled && e->scanBars.blendIntensity > 0.0f;
  case TRANSFORM_PITCH_SPIRAL_BLEND:
    return e->pitchSpiral.enabled && e->pitchSpiral.blendIntensity > 0.0f;
  case TRANSFORM_MULTI_SCALE_GRID:
    return e->multiScaleGrid.enabled;
  case TRANSFORM_MOIRE_GENERATOR_BLEND:
    return e->moireGenerator.enabled && e->moireGenerator.blendIntensity > 0.0f;
  case TRANSFORM_SPECTRAL_ARCS_BLEND:
    return e->spectralArcs.enabled && e->spectralArcs.blendIntensity > 0.0f;
  case TRANSFORM_MUONS_BLEND:
    return e->muons.enabled && e->muons.blendIntensity > 0.0f;
  case TRANSFORM_FILAMENTS_BLEND:
    return e->filaments.enabled && e->filaments.blendIntensity > 0.0f;
  case TRANSFORM_SLASHES_BLEND:
    return e->slashes.enabled && e->slashes.blendIntensity > 0.0f;
  case TRANSFORM_GLYPH_FIELD_BLEND:
    return e->glyphField.enabled && e->glyphField.blendIntensity > 0.0f;
  case TRANSFORM_ARC_STROBE_BLEND:
    return e->arcStrobe.enabled && e->arcStrobe.blendIntensity > 0.0f;
  case TRANSFORM_SIGNAL_FRAMES_BLEND:
    return e->signalFrames.enabled && e->signalFrames.blendIntensity > 0.0f;
  case TRANSFORM_CRT:
    return e->crt.enabled;
  case TRANSFORM_DOT_MATRIX:
    return e->dotMatrix.enabled;
  default:
    return false;
  }
}

#endif // EFFECT_CONFIG_H
