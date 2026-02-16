#include "effect_serialization.h"
#include "config/dual_lissajous_config.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "config/feedback_flow_config.h"
#include "config/procedural_warp_config.h"
#include "config/random_walk_config.h"
#include "effects/anamorphic_streak.h"
#include "effects/arc_strobe.h"
#include "effects/ascii_art.h"
#include "effects/attractor_lines.h"
#include "effects/bit_crush.h"
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
#include "effects/filaments.h"
#include "effects/flux_warp.h"
#include "effects/glitch.h"
#include "effects/glyph_field.h"
#include "effects/gradient_flow.h"
#include "effects/halftone.h"
#include "effects/heightfield_relief.h"
#include "effects/hue_remap.h"
#include "effects/impressionist.h"
#include "effects/infinite_zoom.h"
#include "effects/ink_wash.h"
#include "effects/interference.h"
#include "effects/interference_warp.h"
#include "effects/iris_rings.h"
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
#include "effects/motherboard.h"
#include "effects/multi_scale_grid.h"
#include "effects/muons.h"
#include "effects/nebula.h"
#include "effects/neon_glow.h"
#include "effects/oil_paint.h"
#include "effects/palette_quantization.h"
#include "effects/pencil_sketch.h"
#include "effects/phi_blur.h"
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
#include "effects/tone_warp.h"
#include "effects/toon.h"
#include "effects/triangle_fold.h"
#include "effects/voronoi.h"
#include "effects/watercolor.h"
#include "effects/wave_ripple.h"
#include "render/gradient.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_advection.h"
#include "simulation/curl_flow.h"
#include "simulation/cymatics.h"
#include "simulation/particle_life.h"
#include "simulation/physarum.h"
#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GradientStop, position, color)

void to_json(json &j, const ColorConfig &c) {
  j["mode"] = c.mode;
  switch (c.mode) {
  case COLOR_MODE_SOLID: {
    j["solid"] = c.solid;
  } break;
  case COLOR_MODE_RAINBOW: {
    j["rainbowHue"] = c.rainbowHue;
    j["rainbowRange"] = c.rainbowRange;
    j["rainbowSat"] = c.rainbowSat;
    j["rainbowVal"] = c.rainbowVal;
  } break;
  case COLOR_MODE_GRADIENT: {
    j["gradientStopCount"] = c.gradientStopCount;
    j["gradientStops"] = json::array();
    for (int i = 0; i < c.gradientStopCount; i++) {
      j["gradientStops"].push_back(c.gradientStops[i]);
    }
  } break;
  case COLOR_MODE_PALETTE: {
    j["paletteAR"] = c.paletteAR;
    j["paletteAG"] = c.paletteAG;
    j["paletteAB"] = c.paletteAB;
    j["paletteBR"] = c.paletteBR;
    j["paletteBG"] = c.paletteBG;
    j["paletteBB"] = c.paletteBB;
    j["paletteCR"] = c.paletteCR;
    j["paletteCG"] = c.paletteCG;
    j["paletteCB"] = c.paletteCB;
    j["paletteDR"] = c.paletteDR;
    j["paletteDG"] = c.paletteDG;
    j["paletteDB"] = c.paletteDB;
  } break;
  }
}

void from_json(const json &j, ColorConfig &c) {
  c = ColorConfig{};
  if (j.contains("mode")) {
    c.mode = j["mode"].get<ColorMode>();
  }
  if (j.contains("solid")) {
    c.solid = j["solid"].get<Color>();
  }
  if (j.contains("rainbowHue")) {
    c.rainbowHue = j["rainbowHue"].get<float>();
  }
  if (j.contains("rainbowRange")) {
    c.rainbowRange = j["rainbowRange"].get<float>();
  }
  if (j.contains("rainbowSat")) {
    c.rainbowSat = j["rainbowSat"].get<float>();
  }
  if (j.contains("rainbowVal")) {
    c.rainbowVal = j["rainbowVal"].get<float>();
  }
  if (j.contains("gradientStopCount")) {
    c.gradientStopCount = j["gradientStopCount"].get<int>();
  }
  if (j.contains("gradientStops")) {
    const auto &arr = j["gradientStops"];
    const int count = (int)arr.size();
    for (int i = 0; i < count && i < MAX_GRADIENT_STOPS; i++) {
      c.gradientStops[i] = arr[i].get<GradientStop>();
    }
    c.gradientStopCount =
        (count < MAX_GRADIENT_STOPS) ? count : MAX_GRADIENT_STOPS;
  }

  if (j.contains("paletteAR")) {
    c.paletteAR = j["paletteAR"].get<float>();
  }
  if (j.contains("paletteAG")) {
    c.paletteAG = j["paletteAG"].get<float>();
  }
  if (j.contains("paletteAB")) {
    c.paletteAB = j["paletteAB"].get<float>();
  }
  if (j.contains("paletteBR")) {
    c.paletteBR = j["paletteBR"].get<float>();
  }
  if (j.contains("paletteBG")) {
    c.paletteBG = j["paletteBG"].get<float>();
  }
  if (j.contains("paletteBB")) {
    c.paletteBB = j["paletteBB"].get<float>();
  }
  if (j.contains("paletteCR")) {
    c.paletteCR = j["paletteCR"].get<float>();
  }
  if (j.contains("paletteCG")) {
    c.paletteCG = j["paletteCG"].get<float>();
  }
  if (j.contains("paletteCB")) {
    c.paletteCB = j["paletteCB"].get<float>();
  }
  if (j.contains("paletteDR")) {
    c.paletteDR = j["paletteDR"].get<float>();
  }
  if (j.contains("paletteDG")) {
    c.paletteDG = j["paletteDG"].get<float>();
  }
  if (j.contains("paletteDB")) {
    c.paletteDB = j["paletteDB"].get<float>();
  }

  // Validation: ensure at least 2 stops for gradient mode
  if (c.gradientStopCount < 2) {
    GradientInitDefault(c.gradientStops, &c.gradientStopCount);
  }

  // Ensure stops are sorted by position
  std::sort(c.gradientStops, c.gradientStops + c.gradientStopCount,
            [](const GradientStop &a, const GradientStop &b) {
              return a.position < b.position;
            });
}

// Shared configs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RandomWalkConfig,
                                                RANDOM_WALK_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DualLissajousConfig,
                                                DUAL_LISSAJOUS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FlowFieldConfig,
                                                FLOW_FIELD_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FeedbackFlowConfig,
                                                FEEDBACK_FLOW_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ProceduralWarpConfig,
                                                PROCEDURAL_WARP_CONFIG_FIELDS)

// Simulation configs
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysarumConfig,
                                                PHYSARUM_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CurlFlowConfig,
                                                CURL_FLOW_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AttractorFlowConfig,
                                                ATTRACTOR_FLOW_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BoidsConfig,
                                                BOIDS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CurlAdvectionConfig,
                                                CURL_ADVECTION_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CymaticsConfig,
                                                CYMATICS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParticleLifeConfig,
                                                PARTICLE_LIFE_CONFIG_FIELDS)

// Effect configs A-G
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AnamorphicStreakConfig,
                                                ANAMORPHIC_STREAK_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ArcStrobeConfig,
                                                ARC_STROBE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AsciiArtConfig,
                                                ASCII_ART_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(AttractorLinesConfig,
                                                ATTRACTOR_LINES_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BloomConfig,
                                                BLOOM_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BitCrushConfig,
                                                BIT_CRUSH_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BokehConfig,
                                                BOKEH_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ChladniWarpConfig,
                                                CHLADNI_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CircuitBoardConfig,
                                                CIRCUIT_BOARD_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorGradeConfig,
                                                COLOR_GRADE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConstellationConfig,
                                                CONSTELLATION_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CorridorWarpConfig,
                                                CORRIDOR_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrossHatchingConfig,
                                                CROSS_HATCHING_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CrtConfig, CRT_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    DensityWaveSpiralConfig, DENSITY_WAVE_SPIRAL_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DiscoBallConfig,
                                                DISCO_BALL_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DomainWarpConfig,
                                                DOMAIN_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DotMatrixConfig,
                                                DOT_MATRIX_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DrosteZoomConfig,
                                                DROSTE_ZOOM_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FalseColorConfig,
                                                FALSE_COLOR_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FilamentsConfig,
                                                FILAMENTS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(FluxWarpConfig,
                                                FLUX_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlitchConfig,
                                                GLITCH_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GlyphFieldConfig,
                                                GLYPH_FIELD_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(GradientFlowConfig,
                                                GRADIENT_FLOW_CONFIG_FIELDS)

// Effect configs H-N
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HalftoneConfig,
                                                HALFTONE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    HeightfieldReliefConfig, HEIGHTFIELD_RELIEF_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HueRemapConfig,
                                                HUE_REMAP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ImpressionistConfig,
                                                IMPRESSIONIST_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InfiniteZoomConfig,
                                                INFINITE_ZOOM_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InkWashConfig,
                                                INK_WASH_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterferenceConfig,
                                                INTERFERENCE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(InterferenceWarpConfig,
                                                INTERFERENCE_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IrisRingsConfig,
                                                IRIS_RINGS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KaleidoscopeConfig,
                                                KALEIDOSCOPE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KifsConfig, KIFS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KuwaharaConfig,
                                                KUWAHARA_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LatticeFoldConfig,
                                                LATTICE_FOLD_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LegoBricksConfig,
                                                LEGO_BRICKS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MandelboxConfig,
                                                MANDELBOX_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MatrixRainConfig,
                                                MATRIX_RAIN_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MobiusConfig,
                                                MOBIUS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireLayerConfig,
                                                MOIRE_LAYER_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MoireGeneratorConfig,
                                                MOIRE_GENERATOR_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    MoireInterferenceConfig, MOIRE_INTERFERENCE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MotherboardConfig,
                                                MOTHERBOARD_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MuonsConfig,
                                                MUONS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MultiScaleGridConfig,
                                                MULTI_SCALE_GRID_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NebulaConfig,
                                                NEBULA_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(NeonGlowConfig,
                                                NEON_GLOW_CONFIG_FIELDS)

// Effect configs O-Z
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(OilPaintConfig,
                                                OIL_PAINT_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    PaletteQuantizationConfig, PALETTE_QUANTIZATION_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PencilSketchConfig,
                                                PENCIL_SKETCH_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhiBlurConfig,
                                                PHI_BLUR_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhyllotaxisConfig,
                                                PHYLLOTAXIS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PitchSpiralConfig,
                                                PITCH_SPIRAL_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PixelationConfig,
                                                PIXELATION_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PlasmaConfig,
                                                PLASMA_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PoincareDiskConfig,
                                                POINCARE_DISK_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialIfsConfig,
                                                RADIAL_IFS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialPulseConfig,
                                                RADIAL_PULSE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RadialStreakConfig,
                                                RADIAL_STREAK_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    RelativisticDopplerConfig, RELATIVISTIC_DOPPLER_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ScanBarsConfig,
                                                SCAN_BARS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ShakeConfig,
                                                SHAKE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SignalFramesConfig,
                                                SIGNAL_FRAMES_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SineWarpConfig,
                                                SINE_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SlashesConfig,
                                                SLASHES_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SolidColorConfig,
                                                SOLID_COLOR_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SpectralArcsConfig,
                                                SPECTRAL_ARCS_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SurfaceWarpConfig,
                                                SURFACE_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(SynthwaveConfig,
                                                SYNTHWAVE_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TextureWarpConfig,
                                                TEXTURE_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToneWarpConfig,
                                                TONE_WARP_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ToonConfig, TOON_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriangleFoldConfig,
                                                TRIANGLE_FOLD_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(VoronoiConfig,
                                                VORONOI_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WatercolorConfig,
                                                WATERCOLOR_CONFIG_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(WaveRippleConfig,
                                                WAVE_RIPPLE_CONFIG_FIELDS)

// Look up effect name -> enum value, returns -1 if not found
static int TransformEffectFromName(const char *name) {
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (strcmp(TransformEffectName((TransformEffectType)i), name) == 0) {
      return i;
    }
  }
  return -1;
}

// TransformOrderConfig serialization helpers - called from EffectConfig
// to_json/from_json to_json: Save as string names (stable across enum changes)
static void TransformOrderToJson(json &j, const TransformOrderConfig &t,
                                 const EffectConfig &e) {
  j = json::array();
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (IsTransformEnabled(&e, t.order[i])) {
      j.push_back(TransformEffectName(t.order[i]));
    }
  }
}

// from_json: Merge saved order with defaults - saved effects first, then
// remaining in default order Supports both string names (new) and integer
// indices (legacy)
static void TransformOrderFromJson(const json &j, TransformOrderConfig &t) {
  if (!j.is_array()) {
    t = TransformOrderConfig{};
    return;
  }

  // Track which effects were in saved order
  bool seen[TRANSFORM_EFFECT_COUNT] = {};
  int outIdx = 0;

  // First pass: add saved effects in saved order
  for (size_t i = 0; i < j.size() && outIdx < TRANSFORM_EFFECT_COUNT; i++) {
    int val = -1;
    if (j[i].is_string()) {
      val = TransformEffectFromName(j[i].get<std::string>().c_str());
    } else if (j[i].is_number_integer()) {
      val = j[i].get<int>();
    }
    if (val >= 0 && val < TRANSFORM_EFFECT_COUNT && !seen[val]) {
      t.order[outIdx++] = (TransformEffectType)val;
      seen[val] = true;
    }
  }

  // Second pass: add remaining effects in default order
  const TransformOrderConfig defaultOrder{};
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT && outIdx < TRANSFORM_EFFECT_COUNT;
       i++) {
    const int type = (int)defaultOrder.order[i];
    if (!seen[type]) {
      t.order[outIdx++] = defaultOrder.order[i];
      seen[type] = true;
    }
  }
}

// clang-format off
#define EFFECT_CONFIG_FIELDS(X) \
  X(sineWarp) X(kaleidoscope) X(voronoi) X(physarum) X(curlFlow) \
  X(curlAdvection) X(attractorFlow) X(boids) X(cymatics) X(infiniteZoom) \
  X(interferenceWarp) X(radialStreak) X(relativisticDoppler) X(textureWarp) \
  X(waveRipple) X(mobius) X(pixelation) X(glitch) X(poincareDisk) X(toon) \
  X(heightfieldRelief) X(gradientFlow) X(drosteZoom) X(kifs) X(latticeFold) \
  X(multiScaleGrid) X(colorGrade) X(asciiArt) X(oilPaint) X(watercolor) \
  X(neonGlow) X(radialPulse) X(falseColor) X(halftone) X(dotMatrix) \
  X(chladniWarp) X(corridorWarp) X(crossHatching) X(crt) \
  X(paletteQuantization) X(bokeh) X(bloom) X(anamorphicStreak) X(mandelbox) \
  X(triangleFold) X(radialIfs) X(domainWarp) X(phyllotaxis) \
  X(densityWaveSpiral) X(moireInterference) X(pencilSketch) X(matrixRain) \
  X(impressionist) X(kuwahara) X(legoBricks) X(inkWash) X(discoBall) \
  X(particleLife) X(surfaceWarp) X(shake) X(circuitBoard) X(synthwave) \
  X(constellation) X(plasma) X(interference) X(solidColor) X(toneWarp) \
  X(scanBars) X(pitchSpiral) X(spectralArcs) X(moireGenerator) X(muons) \
  X(filaments) X(slashes) X(glyphField) X(arcStrobe) X(signalFrames) \
  X(nebula) X(motherboard) X(attractorLines) X(phiBlur) X(hueRemap) \
  X(fluxWarp) X(bitCrush) X(irisRings)
// clang-format on

void to_json(json &j, const EffectConfig &e) {
  j["halfLife"] = e.halfLife;
  j["blurScale"] = e.blurScale;
  j["chromaticOffset"] = e.chromaticOffset;
  j["feedbackDesaturate"] = e.feedbackDesaturate;
  j["motionScale"] = e.motionScale;
  j["flowField"] = e.flowField;
  j["feedbackFlow"] = e.feedbackFlow;
  j["proceduralWarp"] = e.proceduralWarp;
  j["gamma"] = e.gamma;
  j["clarity"] = e.clarity;
  TransformOrderToJson(j["transformOrder"], e.transformOrder, e);
#define SERIALIZE_EFFECT(name)                                                 \
  if (e.name.enabled)                                                          \
    j[#name] = e.name;
  EFFECT_CONFIG_FIELDS(SERIALIZE_EFFECT)
#undef SERIALIZE_EFFECT
}

void from_json(const json &j, EffectConfig &e) {
  e = EffectConfig{};
  e.halfLife = j.value("halfLife", e.halfLife);
  e.blurScale = j.value("blurScale", e.blurScale);
  e.chromaticOffset = j.value("chromaticOffset", e.chromaticOffset);
  e.feedbackDesaturate = j.value("feedbackDesaturate", e.feedbackDesaturate);
  e.motionScale = j.value("motionScale", e.motionScale);
  e.flowField = j.value("flowField", e.flowField);
  e.feedbackFlow = j.value("feedbackFlow", e.feedbackFlow);
  e.proceduralWarp = j.value("proceduralWarp", e.proceduralWarp);
  e.gamma = j.value("gamma", e.gamma);
  e.clarity = j.value("clarity", e.clarity);
  if (j.contains("transformOrder")) {
    TransformOrderFromJson(j["transformOrder"], e.transformOrder);
  }
#define DESERIALIZE_EFFECT(name) e.name = j.value(#name, e.name);
  EFFECT_CONFIG_FIELDS(DESERIALIZE_EFFECT)
#undef DESERIALIZE_EFFECT
}
