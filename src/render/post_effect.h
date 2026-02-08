#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include "config/effect_config.h"
#include "effects/anamorphic_streak.h"
#include "effects/ascii_art.h"
#include "effects/bloom.h"
#include "effects/bokeh.h"
#include "effects/chladni_warp.h"
#include "effects/circuit_board.h"
#include "effects/constellation.h"
#include "effects/corridor_warp.h"
#include "effects/cross_hatching.h"
#include "effects/crt.h"
#include "effects/density_wave_spiral.h"
#include "effects/disco_ball.h"
#include "effects/domain_warp.h"
#include "effects/droste_zoom.h"
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
#include "effects/sine_warp.h"
#include "effects/slashes.h"
#include "effects/spectral_arcs.h"
#include "effects/surface_warp.h"
#include "effects/synthwave.h"
#include "effects/texture_warp.h"
#include "effects/toon.h"
#include "effects/triangle_fold.h"
#include "effects/voronoi.h"
#include "effects/watercolor.h"
#include "effects/wave_ripple.h"
#include "raylib.h"
#include <stdint.h>

typedef struct Physarum Physarum;
typedef struct CurlFlow CurlFlow;
typedef struct CurlAdvection CurlAdvection;
typedef struct AttractorFlow AttractorFlow;
typedef struct ParticleLife ParticleLife;
typedef struct Boids Boids;
typedef struct Cymatics Cymatics;
typedef struct BlendCompositor BlendCompositor;
typedef struct ColorLUT ColorLUT;

typedef struct PostEffect {
  RenderTexture2D accumTexture; // Feedback buffer (persists between frames)
  RenderTexture2D pingPong[2];  // Ping-pong buffers for multi-pass effects
  RenderTexture2D
      outputTexture; // Feedback-processed content for textured shape sampling
  Shader feedbackShader;
  Shader blurHShader;
  Shader blurVShader;
  Shader chromaticShader;
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
  int chromaticResolutionLoc;
  int chromaticOffsetLoc;
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
  CurlAdvection *curlAdvection;
  AttractorFlow *attractorFlow;
  ParticleLife *particleLife;
  Boids *boids;
  Cymatics *cymatics;
  SineWarpEffect sineWarp;
  TextureWarpEffect textureWarp;
  WaveRippleEffect waveRipple;
  MobiusEffect mobius;
  GradientFlowEffect gradientFlow;
  ChladniWarpEffect chladniWarp;
  DomainWarpEffect domainWarp;
  SurfaceWarpEffect surfaceWarp;
  InterferenceWarpEffect interferenceWarp;
  CorridorWarpEffect corridorWarp;
  FftRadialWarpEffect fftRadialWarp;
  CircuitBoardEffect circuitBoard;
  RadialPulseEffect radialPulse;
  VoronoiEffect voronoi;
  LatticeFoldEffect latticeFold;
  MultiScaleGridEffect multiScaleGrid;
  PhyllotaxisEffect phyllotaxis;
  KaleidoscopeEffect kaleidoscope;
  KifsEffect kifs;
  PoincareDiskEffect poincareDisk;
  MandelboxEffect mandelbox;
  TriangleFoldEffect triangleFold;
  MoireInterferenceEffect moireInterference;
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
  NeonGlowEffect neonGlow;
  KuwaharaEffect kuwahara;
  HalftoneEffect halftone;
  DiscoBallEffect discoBall;
  LegoBricksEffect legoBricks;
  PixelationEffect pixelation;
  GlitchEffect glitch;
  CrtEffect crt;
  AsciiArtEffect asciiArt;
  MatrixRainEffect matrixRain;
  SynthwaveEffect synthwave;
  BloomEffect bloom;
  BokehEffect bokeh;
  HeightfieldReliefEffect heightfieldRelief;
  AnamorphicStreakEffect anamorphicStreak;
  ColorGradeEffect colorGrade;
  FalseColorEffect falseColor;
  PaletteQuantizationEffect paletteQuantization;
  ConstellationEffect constellation;
  PlasmaEffect plasma;
  InterferenceEffect interference;
  SolidColorEffect solidColor;
  ScanBarsEffect scanBars;
  PitchSpiralEffect pitchSpiral;
  SpectralArcsEffect spectralArcs;
  MoireGeneratorEffect moireGenerator;
  MuonsEffect muons;
  FilamentsEffect filaments;
  SlashesEffect slashes;
  GlyphFieldEffect glyphField;
  BlendCompositor *blendCompositor;
  RenderTexture2D
      generatorScratch;  // Shared scratch texture for generator blend rendering
  Texture2D fftTexture;  // 1D texture (1025x1) for normalized FFT magnitudes
  float fftMaxMagnitude; // Running max for auto-normalization
  Texture2D
      waveformTexture; // 1D texture (2048x1) for waveform history ring buffer
  // Temporaries for RenderPass callbacks
  float currentDeltaTime;
  float currentBlurScale;
  float transformTime; // Shared animation time for transform effects
  float warpTime;
  // Trail boost active state (computed per-frame in RenderPipelineApplyOutput)
  bool physarumBoostActive;
  bool curlFlowBoostActive;
  bool curlAdvectionBoostActive;
  bool attractorFlowBoostActive;
  bool particleLifeBoostActive;
  bool boidsBoostActive;
  bool cymaticsBoostActive;
  // Generator blend active flags (computed per-frame: enabled && blendIntensity
  // > 0)
  bool constellationBlendActive;
  bool plasmaBlendActive;
  bool interferenceBlendActive;
  bool solidColorBlendActive;
  bool scanBarsBlendActive;
  bool pitchSpiralBlendActive;
  bool spectralArcsBlendActive;
  bool moireGeneratorBlendActive;
  bool muonsBlendActive;
  bool filamentsBlendActive;
  bool slashesBlendActive;
  bool glyphFieldBlendActive;
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect *PostEffectInit(int screenWidth, int screenHeight);

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
void PostEffectBeginDrawStage(PostEffect *pe);

// End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

#endif // POST_EFFECT_H
