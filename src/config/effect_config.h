#ifndef EFFECT_CONFIG_H
#define EFFECT_CONFIG_H

#include "effects/anamorphic_streak.h"
#include "effects/arc_strobe.h"
#include "effects/ascii_art.h"
#include "effects/attractor_lines.h"
#include "effects/bit_crush.h"
#include "effects/bloom.h"
#include "effects/bokeh.h"
#include "effects/byzantine.h"
#include "effects/chladni.h"
#include "effects/chladni_warp.h"
#include "effects/chromatic_aberration.h"
#include "effects/circuit_board.h"
#include "effects/color_grade.h"
#include "effects/constellation.h"
#include "effects/corridor_warp.h"
#include "effects/cross_hatching.h"
#include "effects/crt.h"
#include "effects/curl_advection.h"
#include "effects/data_traffic.h"
#include "effects/density_wave_spiral.h"
#include "effects/digital_shard.h"
#include "effects/disco_ball.h"
#include "effects/domain_warp.h"
#include "effects/dot_matrix.h"
#include "effects/droste_zoom.h"
#include "effects/false_color.h"
#include "effects/faraday.h"
#include "effects/filaments.h"
#include "effects/fireworks.h"
#include "effects/flip_book.h"
#include "effects/flux_warp.h"
#include "effects/fracture_grid.h"
#include "effects/galaxy.h"
#include "effects/glitch.h"
#include "effects/glyph_field.h"
#include "effects/gradient_flow.h"
#include "effects/halftone.h"
#include "effects/heightfield_relief.h"
#include "effects/hex_rush.h"
#include "effects/hue_remap.h"
#include "effects/impressionist.h"
#include "effects/infinite_zoom.h"
#include "effects/ink_wash.h"
#include "effects/interference_warp.h"
#include "effects/iris_rings.h"
#include "effects/kaleidoscope.h"
#include "effects/kifs.h"
#include "effects/kuwahara.h"
#include "effects/laser_dance.h"
#include "effects/lattice_crush.h"
#include "effects/lattice_fold.h"
#include "effects/lego_bricks.h"
#include "effects/lens_space.h"
#include "effects/light_medley.h"
#include "effects/mandelbox.h"
#include "effects/matrix_rain.h"
#include "effects/mobius.h"
#include "effects/moire_generator.h"
#include "effects/moire_interference.h"
#include "effects/motherboard.h"
#include "effects/multi_scale_grid.h"
#include "effects/muons.h"
#include "effects/nebula.h"
#include "effects/neon_lattice.h"
#include "effects/oil_paint.h"
#include "effects/palette_quantization.h"
#include "effects/pencil_sketch.h"
#include "effects/perspective_tilt.h"
#include "effects/phi_blur.h"
#include "effects/phyllotaxis.h"
#include "effects/pitch_spiral.h"
#include "effects/pixelation.h"
#include "effects/plaid.h"
#include "effects/plasma.h"
#include "effects/poincare_disk.h"
#include "effects/prism_shatter.h"
#include "effects/radial_ifs.h"
#include "effects/radial_pulse.h"
#include "effects/radial_streak.h"
#include "effects/rainbow_road.h"
#include "effects/relativistic_doppler.h"
#include "effects/ripple_tank.h"
#include "effects/risograph.h"
#include "effects/scan_bars.h"
#include "effects/scrawl.h"
#include "effects/shake.h"
#include "effects/shard_crush.h"
#include "effects/shell.h"
#include "effects/signal_frames.h"
#include "effects/sine_warp.h"
#include "effects/slashes.h"
#include "effects/slit_scan_corridor.h"
#include "effects/solid_color.h"
#include "effects/spark_flash.h"
#include "effects/spectral_arcs.h"
#include "effects/spectral_rings.h"
#include "effects/spin_cage.h"
#include "effects/spiral_walk.h"
#include "effects/stripe_shift.h"
#include "effects/surface_warp.h"
#include "effects/synthwave.h"
#include "effects/texture_warp.h"
#include "effects/tone_warp.h"
#include "effects/toon.h"
#include "effects/triangle_fold.h"
#include "effects/triskelion.h"
#include "effects/twist_tunnel.h"
#include "effects/voronoi.h"
#include "effects/vortex.h"
#include "effects/watercolor.h"
#include "effects/wave_ripple.h"
#include "effects/wave_warp.h"
#include "effects/woodblock.h"
#include "feedback_flow_config.h"
#include "procedural_warp_config.h"
#include "render/blend_mode.h"
#include "simulation/attractor_flow.h"
#include "simulation/boids.h"
#include "simulation/curl_flow.h"
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
  TRANSFORM_PHYSARUM,
  TRANSFORM_CURL_FLOW,
  TRANSFORM_CURL_ADVECTION,
  TRANSFORM_ATTRACTOR_FLOW,
  TRANSFORM_BOIDS,
  TRANSFORM_RIPPLE_TANK,
  TRANSFORM_PARTICLE_LIFE,
  TRANSFORM_DENSITY_WAVE_SPIRAL,
  TRANSFORM_MOIRE_INTERFERENCE,
  TRANSFORM_PENCIL_SKETCH,
  TRANSFORM_PERSPECTIVE_TILT,
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
  TRANSFORM_TONE_WARP,
  TRANSFORM_CONSTELLATION_BLEND,
  TRANSFORM_PLASMA_BLEND,
  TRANSFORM_SOLID_COLOR,
  TRANSFORM_SCAN_BARS_BLEND,
  TRANSFORM_PITCH_SPIRAL_BLEND,
  TRANSFORM_MULTI_SCALE_GRID,
  TRANSFORM_MOIRE_GENERATOR_BLEND,
  TRANSFORM_SPECTRAL_ARCS_BLEND,
  TRANSFORM_MUONS_BLEND,
  TRANSFORM_VORTEX_BLEND,
  TRANSFORM_FILAMENTS_BLEND,
  TRANSFORM_SLASHES_BLEND,
  TRANSFORM_GLYPH_FIELD_BLEND,
  TRANSFORM_ARC_STROBE_BLEND,
  TRANSFORM_SIGNAL_FRAMES_BLEND,
  TRANSFORM_NEBULA_BLEND,
  TRANSFORM_MOTHERBOARD_BLEND,
  TRANSFORM_ATTRACTOR_LINES_BLEND,
  TRANSFORM_CRT,
  TRANSFORM_SHARD_CRUSH,
  TRANSFORM_DOT_MATRIX,
  TRANSFORM_PHI_BLUR,
  TRANSFORM_HUE_REMAP,
  TRANSFORM_FLUX_WARP,
  TRANSFORM_BIT_CRUSH_BLEND,
  TRANSFORM_IRIS_RINGS_BLEND,
  TRANSFORM_DATA_TRAFFIC_BLEND,
  TRANSFORM_FIREWORKS_BLEND,
  TRANSFORM_SPARK_FLASH_BLEND,
  TRANSFORM_LATTICE_CRUSH,
  TRANSFORM_SLIT_SCAN_CORRIDOR,
  TRANSFORM_BYZANTINE_BLEND,
  TRANSFORM_PLAID_BLEND,
  TRANSFORM_PRISM_SHATTER_BLEND,
  TRANSFORM_HEX_RUSH_BLEND,
  TRANSFORM_FRACTURE_GRID,
  TRANSFORM_LENS_SPACE,
  TRANSFORM_SCRAWL_BLEND,
  TRANSFORM_RAINBOW_ROAD_BLEND,
  TRANSFORM_RISOGRAPH,
  TRANSFORM_WOODBLOCK,
  TRANSFORM_CHROMATIC_ABERRATION,
  TRANSFORM_WAVE_WARP,
  TRANSFORM_SPIN_CAGE_BLEND,
  TRANSFORM_SPIRAL_WALK_BLEND,
  TRANSFORM_CHLADNI_BLEND,
  TRANSFORM_FARADAY_BLEND,
  TRANSFORM_GALAXY_BLEND,
  TRANSFORM_SHELL_BLEND,
  TRANSFORM_LASER_DANCE_BLEND,
  TRANSFORM_LIGHT_MEDLEY_BLEND,
  TRANSFORM_DIGITAL_SHARD_BLEND,
  TRANSFORM_SPECTRAL_RINGS_BLEND,
  TRANSFORM_TRISKELION_BLEND,
  TRANSFORM_TWIST_TUNNEL_BLEND,
  TRANSFORM_NEON_LATTICE_BLEND,
  TRANSFORM_FLIP_BOOK,
  TRANSFORM_STRIPE_SHIFT,
  TRANSFORM_ACCUM_COMPOSITE,
  TRANSFORM_EFFECT_COUNT
};

struct TransformOrderConfig {
  TransformEffectType order[TRANSFORM_EFFECT_COUNT];

  TransformOrderConfig();

  TransformEffectType &operator[](int i) { return order[i]; }
  const TransformEffectType &operator[](int i) const { return order[i]; }
};

// Move effect to front of order array (for accum composite default position)
inline void MoveTransformToFront(TransformOrderConfig *config,
                                 TransformEffectType type) {
  int idx = -1;
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    if (config->order[i] == type) {
      idx = i;
      break;
    }
  }
  if (idx <= 0)
    return;
  for (int i = idx; i > 0; i--) {
    config->order[i] = config->order[i - 1];
  }
  config->order[0] = type;
}

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

inline TransformOrderConfig::TransformOrderConfig() {
  for (int i = 0; i < TRANSFORM_EFFECT_COUNT; i++) {
    order[i] = (TransformEffectType)i;
  }
  MoveTransformToFront(this, TRANSFORM_ACCUM_COMPOSITE);
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

#define FLOW_FIELD_CONFIG_FIELDS                                               \
  zoomBase, zoomRadial, rotationSpeed, rotationSpeedRadial, dxBase, dxRadial,  \
      dyBase, dyRadial, cx, cy, sx, sy, zoomAngular, zoomAngularFreq,          \
      rotAngular, rotAngularFreq, dxAngular, dxAngularFreq, dyAngular,         \
      dyAngularFreq

struct EffectConfig {
  float halfLife = 0.5f;  // Trail persistence (seconds)
  float blurScale = 1.0f; // Blur sampling distance (pixels)
  ChromaticAberrationConfig chromaticAberration;
  float feedbackDesaturate = 0.05f; // Fade toward dark gray per frame (0.0-0.2)
  float motionScale = 1.0f;  // Global feedback motion time-dilation (0.01-1.0)
  FlowFieldConfig flowField; // Spatial UV flow field parameters
  FeedbackFlowConfig feedbackFlow;     // Luminance gradient displacement
  ProceduralWarpConfig proceduralWarp; // MilkDrop animated warp distortion
  float gamma = 1.0f;   // Display gamma correction (1.0 = disabled)
  float clarity = 0.0f; // Local contrast enhancement (0.0 = disabled)

  EffectBlendMode accumBlendMode =
      EFFECT_BLEND_SCREEN;           // Accum layer blend operation
  float accumBlendIntensity = 1.0f;  // Accum layer blend strength (0.0-1.0)
  bool accumCompositeEnabled = true; // Always true; never serialized

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

  // Ripple Tank (audio-reactive wave interference from virtual point sources)
  RippleTankConfig rippleTank;

  // Chladni (FFT-driven resonant plate eigenmodes)
  ChladniConfig chladni;

  // Faraday (standing wave patterns on vibrated fluid surface)
  FaradayConfig faraday;

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

  // Wave Warp (wave-basis fBM UV displacement)
  WaveWarpConfig waveWarp;

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

  // Perspective Tilt (3D plane projection with pitch/yaw/roll)
  PerspectiveTiltConfig perspectiveTilt;

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

  // Tone Warp (audio-reactive radial displacement)
  ToneWarpConfig toneWarp;

  // Solid Color (flat color generator with blend)
  SolidColorConfig solidColor;

  // Scan Bars (scrolling luminance bars with blend)
  ScanBarsConfig scanBars;

  // Scrawl (IFS marker curves with scanline grit)
  ScrawlConfig scrawl;

  // Rainbow Road (prismatic highway ribbon generator)
  RainbowRoadConfig rainbowRoad;

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

  // Vortex (raymarched distorted hollow sphere spirals with blend)
  VortexConfig vortex;

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

  // Nebula (FFT-driven procedural nebula clouds with fractal layers and stars)
  NebulaConfig nebula;

  // Motherboard (PCB-trace procedural generator with blend)
  MotherboardConfig motherboard;

  // Dot Matrix (circular dot grid with size/color modulation)
  DotMatrixConfig dotMatrix;

  // Attractor Lines (3D strange attractor trajectories as glowing lines)
  AttractorLinesConfig attractorLines;

  // Phi Blur (golden-ratio directional blur)
  PhiBlurConfig phiBlur;

  // Hue Remap (hue-based gradient remapping via 1D LUT)
  HueRemapConfig hueRemap;

  // Flux Warp
  FluxWarpConfig fluxWarp;

  // Fracture Grid (shattered glass tile warp)
  FractureGridConfig fractureGrid;

  // Bit Crush (iterative lattice walk mosaic generator)
  BitCrushConfig bitCrush;

  // Iris Rings (concentric iris ring generator)
  IrisRingsConfig irisRings;

  // Data Traffic (network packet flow visualization generator)
  DataTrafficConfig dataTraffic;

  // Plaid (tartan weave pattern generator)
  PlaidConfig plaid;

  // Prism Shatter (prismatic light refraction generator)
  PrismShatterConfig prismShatter;

  // Fireworks (audio-reactive particle burst generator)
  FireworksConfig fireworks;

  // Spark Flash (electric spark burst generator)
  SparkFlashConfig sparkFlash;

  // Spin Cage (spinning platonic solid wireframe generator)
  SpinCageConfig spinCage;

  // Spiral Walk (Archimedean spiral path generator)
  SpiralWalkConfig spiralWalk;

  // Hex Rush (Super Hexagon-inspired geometric generator)
  HexRushConfig hexRush;

  // Lattice Crush (lattice-based mosaic transform)
  LatticeCrushConfig latticeCrush;

  // Lens Space (recursive L(p,q) crystal-ball raymarching)
  LensSpaceConfig lensSpace;

  // Slit Scan Corridor (slit-sampled perspective tunnel via ping-pong
  // accumulation)
  SlitScanCorridorConfig slitScanCorridor;

  // Byzantine (reaction-diffusion labyrinthine texture with fractal zoom)
  ByzantineConfig byzantine;

  // Risograph (CMY ink separation with grain and misregistration)
  RisographConfig risograph;

  // Woodblock (ukiyo-e posterization with keyblock outlines)
  WoodblockConfig woodblock;

  // Galaxy (spiral galaxy generator with density wave arms)
  GalaxyConfig galaxy;

  // Laser Dance (filament laser beam generator)
  LaserDanceConfig laserDance;

  // Light Medley
  LightMedleyConfig lightMedley;

  // Shell
  ShellConfig shell;

  // Digital Shard (shattered glass prism generator)
  DigitalShardConfig digitalShard;

  // Shard Crush (angular block shattering with chromatic aberration)
  ShardCrushConfig shardCrush;

  // Spectral Rings (concentric spectral ring generator)
  SpectralRingsConfig spectralRings;

  // Triskelion (fractal tiled grid with space-folding and circle interference)
  TriskelionConfig triskelion;

  // Twist Tunnel (twisted tunnel generator)
  TwistTunnelConfig twistTunnel;

  // Neon Lattice (raymarched infinite torus lattice generator)
  NeonLatticeConfig neonLattice;

  // Flip Book
  FlipBookConfig flipBook;

  // Stripe Shift (flat RGB bars displaced by input brightness)
  StripeShiftConfig stripeShift;

  // Transform effect execution order
  TransformOrderConfig transformOrder;
};

#endif // EFFECT_CONFIG_H
