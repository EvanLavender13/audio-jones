#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include "config/effect_config.h"
#include "effects/anamorphic_streak.h"
#include "effects/arc_strobe.h"
#include "effects/ascii_art.h"
#include "effects/attractor_lines.h"
#include "effects/bilateral.h"
#include "effects/bit_crush.h"
#include "effects/bloom.h"
#include "effects/bokeh.h"
#include "effects/byzantine.h"
#include "effects/chladni.h"
#include "effects/chladni_warp.h"
#include "effects/chromatic_aberration.h"
#include "effects/circuit_board.h"
#include "effects/color_stretch.h"
#include "effects/constellation.h"
#include "effects/corridor_warp.h"
#include "effects/cross_hatching.h"
#include "effects/crt.h"
#include "effects/cyber_march.h"
#include "effects/data_traffic.h"
#include "effects/density_wave_spiral.h"
#include "effects/digital_shard.h"
#include "effects/disco_ball.h"
#include "effects/domain_warp.h"
#include "effects/dot_matrix.h"
#include "effects/dream_fractal.h"
#include "effects/droste_zoom.h"
#include "effects/faraday.h"
#include "effects/filaments.h"
#include "effects/fireworks.h"
#include "effects/flip_book.h"
#include "effects/flux_warp.h"
#include "effects/fractal_tree.h"
#include "effects/fracture_grid.h"
#include "effects/galaxy.h"
#include "effects/glitch.h"
#include "effects/glyph_field.h"
#include "effects/gradient_flow.h"
#include "effects/halftone.h"
#include "effects/hex_rush.h"
#include "effects/hue_remap.h"
#include "effects/impressionist.h"
#include "effects/infinite_zoom.h"
#include "effects/infinity_matrix.h"
#include "effects/ink_wash.h"
#include "effects/interference_warp.h"
#include "effects/iris_rings.h"
#include "effects/isoflow.h"
#include "effects/kaleidoscope.h"
#include "effects/kifs.h"
#include "effects/kuwahara.h"
#include "effects/laser_dance.h"
#include "effects/lattice_crush.h"
#include "effects/lattice_fold.h"
#include "effects/lego_bricks.h"
#include "effects/lens_space.h"
#include "effects/light_medley.h"
#include "effects/lotus_warp.h"
#include "effects/mandelbox.h"
#include "effects/marble.h"
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
#include "effects/orrery.h"
#include "effects/pencil_sketch.h"
#include "effects/perspective_tilt.h"
#include "effects/phi_blur.h"
#include "effects/phyllotaxis.h"
#include "effects/pitch_spiral.h"
#include "effects/pixelation.h"
#include "effects/plaid.h"
#include "effects/plasma.h"
#include "effects/poincare_disk.h"
#include "effects/polygon_subdivide.h"
#include "effects/polyhedral_mirror.h"
#include "effects/polymorph.h"
#include "effects/prism_shatter.h"
#include "effects/protean_clouds.h"
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
#include "effects/slashes.h"
#include "effects/slit_scan.h"
#include "effects/solarize.h"
#include "effects/spark_flash.h"
#include "effects/spectral_arcs.h"
#include "effects/spectral_rings.h"
#include "effects/spin_cage.h"
#include "effects/spiral_nest.h"
#include "effects/spiral_walk.h"
#include "effects/star_trail.h"
#include "effects/stripe_shift.h"
#include "effects/subdivide.h"
#include "effects/surface_depth.h"
#include "effects/surface_warp.h"
#include "effects/synapse_tree.h"
#include "effects/synthwave.h"
#include "effects/texture_warp.h"
#include "effects/tone_warp.h"
#include "effects/toon.h"
#include "effects/triangle_fold.h"
#include "effects/triskelion.h"
#include "effects/twist_cage.h"
#include "effects/vignette.h"
#include "effects/viscera.h"
#include "effects/voronoi.h"
#include "effects/vortex.h"
#include "effects/voxel_march.h"
#include "effects/watercolor.h"
#include "effects/wave_drift.h"
#include "effects/wave_ripple.h"
#include "effects/wave_warp.h"
#include "effects/woodblock.h"
#include "raylib.h"
#include <stdint.h>

typedef struct Physarum Physarum;
typedef struct CurlFlow CurlFlow;
typedef struct AttractorFlow AttractorFlow;
typedef struct ParticleLife ParticleLife;
typedef struct Boids Boids;
typedef struct MazeWorms MazeWorms;
typedef struct BlendCompositor BlendCompositor;
typedef struct ColorLUT ColorLUT;

// Progress callback type - called between init phases
// progress: 0.0 to 1.0
typedef void (*PostEffectProgressFn)(float progress, void *userData);

typedef struct PostEffect {
  RenderTexture2D accumTexture; // Feedback buffer (persists between frames)
  RenderTexture2D pingPong[2];  // Ping-pong buffers for multi-pass effects
  RenderTexture2D
      outputTexture; // Feedback-processed content for textured shape sampling
  Shader feedbackShader;
  Shader blurHShader;
  Shader blurVShader;
  ChromaticAberrationEffect chromaticAberration;
  VignetteEffect vignette;
  Shader fxaaShader;
  Shader clarityShader;
  Shader gammaShader;
  Shader shapeTextureShader;
  RenderTexture2D halfResA;
  RenderTexture2D halfResB;
  int shapeTexZoomLoc;
  int shapeTexAngleLoc;
  int shapeTexBrightnessLoc;
  int blurHResolutionLoc;
  int blurVResolutionLoc;
  int blurHScaleLoc;
  int blurVScaleLoc;
  int halfLifeLoc;
  int deltaTimeLoc;
  int feedbackResolutionLoc;
  int feedbackDesaturateLoc;
  int feedbackZoomBaseLoc;
  int feedbackZoomRadialLoc;
  int feedbackRotBaseLoc;
  int feedbackRotRadialLoc;
  int feedbackDxBaseLoc;
  int feedbackDxRadialLoc;
  int feedbackDyBaseLoc;
  int feedbackDyRadialLoc;
  int feedbackFlowStrengthLoc;
  int feedbackFlowAngleLoc;
  int feedbackFlowScaleLoc;
  int feedbackFlowThresholdLoc;
  int feedbackCxLoc;
  int feedbackCyLoc;
  int feedbackSxLoc;
  int feedbackSyLoc;
  int feedbackZoomAngularLoc;
  int feedbackZoomAngularFreqLoc;
  int feedbackRotAngularLoc;
  int feedbackRotAngularFreqLoc;
  int feedbackDxAngularLoc;
  int feedbackDxAngularFreqLoc;
  int feedbackDyAngularLoc;
  int feedbackDyAngularFreqLoc;
  int feedbackWarpLoc;
  int feedbackWarpTimeLoc;
  int feedbackWarpScaleInverseLoc;
  int fxaaResolutionLoc;
  int clarityResolutionLoc;
  int clarityAmountLoc;
  int gammaGammaLoc;
  EffectConfig effects;
  int screenWidth;
  int screenHeight;
  Physarum *physarum;
  CurlFlow *curlFlow;
  CurlAdvectionEffect curlAdvection;
  CyberMarchEffect cyberMarch;
  AttractorFlow *attractorFlow;
  ParticleLife *particleLife;
  Boids *boids;
  MazeWorms *mazeWorms;
  WaveDriftEffect waveDrift;
  TextureWarpEffect textureWarp;
  WaveRippleEffect waveRipple;
  MobiusEffect mobius;
  GradientFlowEffect gradientFlow;
  ChladniWarpEffect chladniWarp;
  DomainWarpEffect domainWarp;
  SurfaceWarpEffect surfaceWarp;
  InterferenceWarpEffect interferenceWarp;
  CorridorWarpEffect corridorWarp;
  LensSpaceEffect lensSpace;
  ToneWarpEffect toneWarp;
  FluxWarpEffect fluxWarp;
  FractalTreeEffect fractalTree;
  FractureGridEffect fractureGrid;
  CircuitBoardEffect circuitBoard;
  RadialPulseEffect radialPulse;
  VoronoiEffect voronoi;
  LatticeFoldEffect latticeFold;
  LatticeCrushEffect latticeCrush;
  MultiScaleGridEffect multiScaleGrid;
  PhyllotaxisEffect phyllotaxis;
  LotusWarpEffect lotusWarp;
  KaleidoscopeEffect kaleidoscope;
  KifsEffect kifs;
  PoincareDiskEffect poincareDisk;
  MandelboxEffect mandelbox;
  TriangleFoldEffect triangleFold;
  WaveWarpEffect waveWarp;
  MoireInterferenceEffect moireInterference;
  MotherboardEffect motherboard;
  RadialIfsEffect radialIfs;
  InfiniteZoomEffect infiniteZoom;
  RadialStreakEffect radialStreak;
  DrosteZoomEffect drosteZoom;
  DensityWaveSpiralEffect densityWaveSpiral;
  ShakeEffect shake;
  RelativisticDopplerEffect relativisticDoppler;
  OilPaintEffect oilPaint;
  WatercolorEffect watercolor;
  ImpressionistEffect impressionist;
  InkWashEffect inkWash;
  PencilSketchEffect pencilSketch;
  CrossHatchingEffect crossHatching;
  ToonEffect toon;
  KuwaharaEffect kuwahara;
  BilateralEffect bilateral;
  HalftoneEffect halftone;
  DiscoBallEffect discoBall;
  DotMatrixEffect dotMatrix;
  LegoBricksEffect legoBricks;
  RisographEffect risograph;
  WoodblockEffect woodblock;
  PixelationEffect pixelation;
  GlitchEffect glitch;
  BitCrushEffect bitCrush;
  CrtEffect crt;
  AsciiArtEffect asciiArt;
  MatrixRainEffect matrixRain;
  SynthwaveEffect synthwave;
  BloomEffect bloom;
  BokehEffect bokeh;
  SurfaceDepthEffect surfaceDepth;
  PerspectiveTiltEffect perspectiveTilt;
  PhiBlurEffect phiBlur;
  AnamorphicStreakEffect anamorphicStreak;
  ColorGradeEffect colorGrade;
  FalseColorEffect falseColor;
  HueRemapEffect hueRemap;
  SolarizeEffect solarize;
  PaletteQuantizationEffect paletteQuantization;
  ConstellationEffect constellation;
  DataTrafficEffect dataTraffic;
  PlasmaEffect plasma;
  IrisRingsEffect irisRings;
  SolidColorEffect solidColor;
  ScanBarsEffect scanBars;
  ScrawlEffect scrawl;
  PitchSpiralEffect pitchSpiral;
  SpectralArcsEffect spectralArcs;
  SpectralRingsEffect spectralRings;
  SignalFramesEffect signalFrames;
  HexRushEffect hexRush;
  SubdivideEffect subdivide;
  PolygonSubdivideEffect polygonSubdivide;
  MoireGeneratorEffect moireGenerator;
  MuonsEffect muons;
  VortexEffect vortex;
  SynapseTreeEffect synapseTree;
  FilamentsEffect filaments;
  ArcStrobeEffect arcStrobe;
  SlashesEffect slashes;
  GlyphFieldEffect glyphField;
  NebulaEffect nebula;
  DreamFractalEffect dreamFractal;
  AttractorLinesEffect attractorLines;
  FireworksEffect fireworks;
  FlipBookEffect flipBook;
  SparkFlashEffect sparkFlash;
  SpinCageEffect spinCage;
  PolymorphEffect polymorph;
  PolyhedralMirrorEffect polyhedralMirror;
  SpiralWalkEffect spiralWalk;
  SpiralNestEffect spiralNest;
  StarTrailEffect starTrail;
  RippleTankEffect rippleTank;
  ChladniEffect chladni;
  FaradayEffect faraday;
  GalaxyEffect galaxy;
  LaserDanceEffect laserDance;
  LightMedleyEffect lightMedley;
  VoxelMarchEffect voxelMarch;
  IsoflowEffect isoflow;
  ProteanCloudsEffect proteanClouds;
  ShellEffect shell;
  VisceraEffect viscera;
  DigitalShardEffect digitalShard;
  ShardCrushEffect shardCrush;
  PlaidEffect plaid;
  PrismShatterEffect prismShatter;
  RainbowRoadEffect rainbowRoad;
  SlitScanEffect slitScan;
  ByzantineEffect byzantine;
  TriskelionEffect triskelion;
  TwistCageEffect twistCage;
  NeonLatticeEffect neonLattice;
  OrreryEffect orrery;
  ColorStretchEffect colorStretch;
  InfinityMatrixEffect infinityMatrix;
  MarbleEffect marble;
  StripeShiftEffect stripeShift;
  BlendCompositor *blendCompositor;
  RenderTexture2D
      generatorScratch;  // Shared scratch texture for generator blend rendering
  Texture2D fftTexture;  // 1D texture (1025x1) for normalized FFT magnitudes
  float fftMaxMagnitude; // Running max for auto-normalization
  Texture2D
      waveformTexture; // 1D texture (2048x1) for waveform history ring buffer
  int waveformWriteIndex;
  // Temporaries for RenderPass callbacks
  float currentDeltaTime;
  float currentBlurScale;
  float transformTime; // Shared animation time for transform effects
  float warpTime;
  Texture2D currentSceneTexture;
  RenderTexture2D
      *currentRenderDest; // Pipeline output for custom render effects
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect *PostEffectInit(int screenWidth, int screenHeight,
                           PostEffectProgressFn onProgress, void *userData);

// Clean up post-effect resources
void PostEffectUninit(PostEffect *pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect *pe, int width, int height);

// Register per-effect params with the modulation engine
// Called after ParamRegistryInit; individual effects add their params here
void PostEffectRegisterParams(PostEffect *pe);

// Clear feedback buffers and reset simulations (call when switching presets)
void PostEffectClearFeedback(PostEffect *pe);

// Begin drawing waveforms to accumulation texture
void PostEffectBeginDrawStage(const PostEffect *pe);

// End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

#endif // POST_EFFECT_H
