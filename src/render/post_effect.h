#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include "config/effect_config.h"
#include "effects/chladni_warp.h"
#include "effects/circuit_board.h"
#include "effects/corridor_warp.h"
#include "effects/cross_hatching.h"
#include "effects/density_wave_spiral.h"
#include "effects/domain_warp.h"
#include "effects/droste_zoom.h"
#include "effects/fft_radial_warp.h"
#include "effects/gradient_flow.h"
#include "effects/impressionist.h"
#include "effects/infinite_zoom.h"
#include "effects/ink_wash.h"
#include "effects/interference_warp.h"
#include "effects/kaleidoscope.h"
#include "effects/kifs.h"
#include "effects/lattice_fold.h"
#include "effects/mandelbox.h"
#include "effects/mobius.h"
#include "effects/moire_interference.h"
#include "effects/oil_paint.h"
#include "effects/pencil_sketch.h"
#include "effects/phyllotaxis.h"
#include "effects/poincare_disk.h"
#include "effects/radial_ifs.h"
#include "effects/radial_pulse.h"
#include "effects/radial_streak.h"
#include "effects/relativistic_doppler.h"
#include "effects/shake.h"
#include "effects/sine_warp.h"
#include "effects/surface_warp.h"
#include "effects/texture_warp.h"
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
  Shader pixelationShader;
  Shader plasmaShader;
  Shader glitchShader;
  Shader toonShader;
  Shader heightfieldReliefShader;
  Shader colorGradeShader;
  Shader constellationShader;
  Shader asciiArtShader;
  Shader neonGlowShader;
  Shader falseColorShader;
  Shader halftoneShader;
  Shader paletteQuantizationShader;
  Shader bokehShader;
  Shader bloomPrefilterShader;
  Shader bloomDownsampleShader;
  Shader bloomUpsampleShader;
  Shader bloomCompositeShader;
  Shader anamorphicStreakPrefilterShader;
  Shader anamorphicStreakBlurShader;
  Shader anamorphicStreakCompositeShader;
  Shader matrixRainShader;
  Shader kuwaharaShader;
  Shader discoBallShader;
  Shader legoBricksShader;
  Shader synthwaveShader;
  Shader interferenceShader;
  RenderTexture2D bloomMips[BLOOM_MIP_COUNT];
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
  int pixelationResolutionLoc;
  int pixelationCellCountLoc;
  int pixelationDitherScaleLoc;
  int pixelationPosterizeLevelsLoc;
  // Plasma
  int plasmaResolutionLoc;
  int plasmaAnimPhaseLoc;
  int plasmaDriftPhaseLoc;
  int plasmaFlickerTimeLoc;
  int plasmaBoltCountLoc;
  int plasmaLayerCountLoc;
  int plasmaOctavesLoc;
  int plasmaFalloffTypeLoc;
  int plasmaDriftAmountLoc;
  int plasmaDisplacementLoc;
  int plasmaGlowRadiusLoc;
  int plasmaCoreBrightnessLoc;
  int plasmaFlickerAmountLoc;
  int plasmaGradientLUTLoc;
  int glitchResolutionLoc;
  int glitchTimeLoc;
  int glitchFrameLoc;
  int glitchCrtEnabledLoc;
  int glitchCurvatureLoc;
  int glitchVignetteEnabledLoc;
  int glitchAnalogIntensityLoc;
  int glitchAberrationLoc;
  int glitchBlockThresholdLoc;
  int glitchBlockOffsetLoc;
  int glitchVhsEnabledLoc;
  int glitchTrackingBarIntensityLoc;
  int glitchScanlineNoiseIntensityLoc;
  int glitchColorDriftIntensityLoc;
  int glitchScanlineAmountLoc;
  int glitchNoiseAmountLoc;
  int glitchDatamoshEnabledLoc;
  int glitchDatamoshIntensityLoc;
  int glitchDatamoshMinLoc;
  int glitchDatamoshMaxLoc;
  int glitchDatamoshSpeedLoc;
  int glitchDatamoshBandsLoc;
  int glitchRowSliceEnabledLoc;
  int glitchRowSliceIntensityLoc;
  int glitchRowSliceBurstFreqLoc;
  int glitchRowSliceBurstPowerLoc;
  int glitchRowSliceColumnsLoc;
  int glitchColSliceEnabledLoc;
  int glitchColSliceIntensityLoc;
  int glitchColSliceBurstFreqLoc;
  int glitchColSliceBurstPowerLoc;
  int glitchColSliceRowsLoc;
  int glitchDiagonalBandsEnabledLoc;
  int glitchDiagonalBandCountLoc;
  int glitchDiagonalBandDisplaceLoc;
  int glitchDiagonalBandSpeedLoc;
  int glitchBlockMaskEnabledLoc;
  int glitchBlockMaskIntensityLoc;
  int glitchBlockMaskMinSizeLoc;
  int glitchBlockMaskMaxSizeLoc;
  int glitchBlockMaskTintLoc;
  int glitchTemporalJitterEnabledLoc;
  int glitchTemporalJitterAmountLoc;
  int glitchTemporalJitterGateLoc;
  int glitchBlockMultiplyEnabledLoc;
  int glitchBlockMultiplySizeLoc;
  int glitchBlockMultiplyControlLoc;
  int glitchBlockMultiplyIterationsLoc;
  int glitchBlockMultiplyIntensityLoc;
  int toonResolutionLoc;
  int toonLevelsLoc;
  int toonEdgeThresholdLoc;
  int toonEdgeSoftnessLoc;
  int toonThicknessVariationLoc;
  int toonNoiseScaleLoc;
  int heightfieldReliefResolutionLoc;
  int heightfieldReliefIntensityLoc;
  int heightfieldReliefReliefScaleLoc;
  int heightfieldReliefLightAngleLoc;
  int heightfieldReliefLightHeightLoc;
  int heightfieldReliefShininessLoc;
  int colorGradeHueShiftLoc;
  int colorGradeSaturationLoc;
  int colorGradeBrightnessLoc;
  int colorGradeContrastLoc;
  int colorGradeTemperatureLoc;
  int colorGradeShadowsOffsetLoc;
  int colorGradeMidtonesOffsetLoc;
  int colorGradeHighlightsOffsetLoc;
  int constellationPointSizeLoc;
  int constellationGridScaleLoc;
  int constellationInterpolateLineColorLoc;
  int constellationLineLUTLoc;
  int constellationLineOpacityLoc;
  int constellationLineThicknessLoc;
  int constellationMaxLineLenLoc;
  int constellationPointBrightnessLoc;
  int constellationPointLUTLoc;
  int constellationRadialAmpLoc;
  int constellationRadialFreqLoc;
  int constellationResolutionLoc;
  int constellationAnimPhaseLoc;
  int constellationRadialPhaseLoc;
  int constellationWanderAmpLoc;
  int asciiArtResolutionLoc;
  int asciiArtCellPixelsLoc;
  int asciiArtColorModeLoc;
  int asciiArtForegroundLoc;
  int asciiArtBackgroundLoc;
  int asciiArtInvertLoc;
  int neonGlowResolutionLoc;
  int neonGlowGlowColorLoc;
  int neonGlowEdgeThresholdLoc;
  int neonGlowEdgePowerLoc;
  int neonGlowGlowIntensityLoc;
  int neonGlowGlowRadiusLoc;
  int neonGlowGlowSamplesLoc;
  int neonGlowOriginalVisibilityLoc;
  int neonGlowColorModeLoc;
  int neonGlowSaturationBoostLoc;
  int neonGlowBrightnessBoostLoc;
  int falseColorIntensityLoc;
  int falseColorGradientLUTLoc;
  int halftoneResolutionLoc;
  int halftoneDotScaleLoc;
  int halftoneDotSizeLoc;
  int halftoneRotationLoc;
  int paletteQuantizationColorLevelsLoc;
  int paletteQuantizationDitherStrengthLoc;
  int paletteQuantizationBayerSizeLoc;
  int bokehResolutionLoc;
  int bokehRadiusLoc;
  int bokehIterationsLoc;
  int bokehBrightnessPowerLoc;
  int bloomThresholdLoc;
  int bloomKneeLoc;
  int bloomDownsampleHalfpixelLoc;
  int bloomUpsampleHalfpixelLoc;
  int bloomIntensityLoc;
  int bloomBloomTexLoc;
  int anamorphicStreakThresholdLoc;
  int anamorphicStreakKneeLoc;
  int anamorphicStreakResolutionLoc;
  int anamorphicStreakOffsetLoc;
  int anamorphicStreakSharpnessLoc;
  int anamorphicStreakIntensityLoc;
  int anamorphicStreakStreakTexLoc;
  int matrixRainResolutionLoc;
  int matrixRainCellSizeLoc;
  int matrixRainTrailLengthLoc;
  int matrixRainFallerCountLoc;
  int matrixRainOverlayIntensityLoc;
  int matrixRainRefreshRateLoc;
  int matrixRainLeadBrightnessLoc;
  int matrixRainTimeLoc;
  int matrixRainSampleModeLoc;
  int kuwaharaResolutionLoc;
  int kuwaharaRadiusLoc;
  int discoBallResolutionLoc;
  int discoBallSphereRadiusLoc;
  int discoBallTileSizeLoc;
  int discoBallSphereAngleLoc;
  int discoBallBumpHeightLoc;
  int discoBallReflectIntensityLoc;
  int discoBallSpotIntensityLoc;
  int discoBallSpotFalloffLoc;
  int discoBallBrightnessThresholdLoc;
  int legoBricksResolutionLoc;
  int legoBricksBrickScaleLoc;
  int legoBricksStudHeightLoc;
  int legoBricksEdgeShadowLoc;
  int legoBricksColorThresholdLoc;
  int legoBricksMaxBrickSizeLoc;
  int legoBricksLightAngleLoc;
  int synthwaveResolutionLoc;
  int synthwaveHorizonYLoc;
  int synthwaveColorMixLoc;
  int synthwavePalettePhaseLoc;
  int synthwaveGridSpacingLoc;
  int synthwaveGridThicknessLoc;
  int synthwaveGridOpacityLoc;
  int synthwaveGridGlowLoc;
  int synthwaveGridColorLoc;
  int synthwaveStripeCountLoc;
  int synthwaveStripeSoftnessLoc;
  int synthwaveStripeIntensityLoc;
  int synthwaveSunColorLoc;
  int synthwaveHorizonIntensityLoc;
  int synthwaveHorizonFalloffLoc;
  int synthwaveHorizonColorLoc;
  int synthwaveGridTimeLoc;
  int synthwaveStripeTimeLoc;
  int interferenceResolutionLoc;
  int interferenceTimeLoc;
  int interferenceSourcesLoc;
  int interferencePhasesLoc;
  int interferenceWaveFreqLoc;
  int interferenceFalloffTypeLoc;
  int interferenceFalloffStrengthLoc;
  int interferenceBoundariesLoc;
  int interferenceReflectionGainLoc;
  int interferenceVisualModeLoc;
  int interferenceContourCountLoc;
  int interferenceVisualGainLoc;
  int interferenceChromaSpreadLoc;
  int interferenceColorModeLoc;
  int interferenceColorLUTLoc;
  int interferenceSourceCountLoc;
  EffectConfig effects;
  int screenWidth;
  int screenHeight;
  float synthwaveGridTime;
  float synthwaveStripeTime;
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
  BlendCompositor *blendCompositor;
  ColorLUT *constellationLineLUT;
  ColorLUT *constellationPointLUT;
  ColorLUT *falseColorLUT; // 1D gradient texture for false color effect
  ColorLUT *plasmaGradientLUT;
  ColorLUT *interferenceColorLUT;
  Texture2D fftTexture;  // 1D texture (1025x1) for normalized FFT magnitudes
  float fftMaxMagnitude; // Running max for auto-normalization
  Texture2D
      waveformTexture; // 1D texture (2048x1) for waveform history ring buffer
  // Temporaries for RenderPass callbacks
  float currentDeltaTime;
  float currentBlurScale;
  float transformTime; // Shared animation time for transform effects
  float glitchTime;
  int glitchFrame;
  float currentHalftoneRotation;
  float warpTime;
  float matrixRainTime;
  float discoBallAngle;
  float constellationAnimPhase;
  float constellationRadialPhase;
  // Plasma
  float plasmaAnimPhase;
  float plasmaDriftPhase;
  float plasmaFlickerTime;
  // Interference
  float interferenceTime; // Wave animation accumulator
  // Trail boost active state (computed per-frame in RenderPipelineApplyOutput)
  bool physarumBoostActive;
  bool curlFlowBoostActive;
  bool curlAdvectionBoostActive;
  bool attractorFlowBoostActive;
  bool particleLifeBoostActive;
  bool boidsBoostActive;
  bool cymaticsBoostActive;
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect *PostEffectInit(int screenWidth, int screenHeight);

// Clean up post-effect resources
void PostEffectUninit(PostEffect *pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect *pe, int width, int height);

// Clear feedback buffers and reset simulations (call when switching presets)
void PostEffectClearFeedback(PostEffect *pe);

// Begin drawing waveforms to accumulation texture
void PostEffectBeginDrawStage(PostEffect *pe);

// End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

#endif // POST_EFFECT_H
