#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include <stdint.h>
#include "raylib.h"
#include "config/effect_config.h"

typedef struct Physarum Physarum;
typedef struct CurlFlow CurlFlow;
typedef struct AttractorFlow AttractorFlow;
typedef struct BlendCompositor BlendCompositor;

typedef struct PostEffect {
    RenderTexture2D accumTexture;     // Feedback buffer (persists between frames)
    RenderTexture2D pingPong[2];      // Ping-pong buffers for multi-pass effects
    RenderTexture2D outputTexture;    // Previous frame's final output (1-frame delay) for textured shapes
    Shader feedbackShader;
    Shader blurHShader;
    Shader blurVShader;
    Shader chromaticShader;
    Shader kaleidoShader;
    Shader voronoiShader;
    Shader fxaaShader;
    Shader clarityShader;
    Shader gammaShader;
    Shader shapeTextureShader;
    Shader infiniteZoomShader;
    Shader sineWarpShader;
    Shader radialStreakShader;
    Shader textureWarpShader;
    Shader waveRippleShader;
    Shader mobiusShader;
    Shader pixelationShader;
    Shader glitchShader;
    Shader poincareDiskShader;
    Shader toonShader;
    Shader heightfieldReliefShader;
    Shader gradientFlowShader;
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
    int kaleidoSegmentsLoc;
    int kaleidoRotationLoc;
    int kaleidoTimeLoc;
    int kaleidoTwistLoc;
    int kaleidoFocalLoc;
    int kaleidoWarpStrengthLoc;
    int kaleidoWarpSpeedLoc;
    int kaleidoNoiseScaleLoc;
    int kaleidoPolarIntensityLoc;
    int kaleidoKifsIntensityLoc;
    int kaleidoIterMirrorIntensityLoc;
    int kaleidoHexFoldIntensityLoc;
    int kaleidoKifsIterationsLoc;
    int kaleidoKifsScaleLoc;
    int kaleidoKifsOffsetLoc;
    int kaleidoHexScaleLoc;
    int voronoiResolutionLoc;
    int voronoiScaleLoc;
    int voronoiTimeLoc;
    int voronoiEdgeFalloffLoc;
    int voronoiIsoFrequencyLoc;
    int voronoiUvDistortIntensityLoc;
    int voronoiEdgeIsoIntensityLoc;
    int voronoiCenterIsoIntensityLoc;
    int voronoiFlatFillIntensityLoc;
    int voronoiEdgeDarkenIntensityLoc;
    int voronoiAngleShadeIntensityLoc;
    int voronoiDeterminantIntensityLoc;
    int voronoiRatioIntensityLoc;
    int voronoiEdgeDetectIntensityLoc;
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
    int fxaaResolutionLoc;
    int clarityResolutionLoc;
    int clarityAmountLoc;
    int gammaGammaLoc;
    int infiniteZoomTimeLoc;
    int infiniteZoomZoomDepthLoc;
    int infiniteZoomLayersLoc;
    int infiniteZoomSpiralAngleLoc;
    int infiniteZoomSpiralTwistLoc;
    int infiniteZoomDrosteShearLoc;
    int sineWarpTimeLoc;
    int sineWarpOctavesLoc;
    int sineWarpStrengthLoc;
    int sineWarpOctaveRotationLoc;
    int sineWarpUvScaleLoc;
    int radialStreakSamplesLoc;
    int radialStreakStreakLengthLoc;
    int textureWarpStrengthLoc;
    int textureWarpIterationsLoc;
    int textureWarpChannelModeLoc;
    int waveRippleTimeLoc;
    int waveRippleOctavesLoc;
    int waveRippleStrengthLoc;
    int waveRippleFrequencyLoc;
    int waveRippleSteepnessLoc;
    int waveRippleOriginLoc;
    int waveRippleShadeEnabledLoc;
    int waveRippleShadeIntensityLoc;
    int mobiusTimeLoc;
    int mobiusPoint1Loc;
    int mobiusPoint2Loc;
    int mobiusSpiralTightnessLoc;
    int mobiusZoomFactorLoc;
    int pixelationResolutionLoc;
    int pixelationCellCountLoc;
    int pixelationDitherScaleLoc;
    int pixelationPosterizeLevelsLoc;
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
    int poincareDiskTilePLoc;
    int poincareDiskTileQLoc;
    int poincareDiskTileRLoc;
    int poincareDiskTranslationLoc;
    int poincareDiskRotationLoc;
    int poincareDiskDiskScaleLoc;
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
    int gradientFlowResolutionLoc;
    int gradientFlowStrengthLoc;
    int gradientFlowIterationsLoc;
    int gradientFlowFlowAngleLoc;
    int gradientFlowEdgeWeightLoc;
    EffectConfig effects;
    int screenWidth;
    int screenHeight;
    float voronoiTime;
    Physarum* physarum;
    CurlFlow* curlFlow;
    AttractorFlow* attractorFlow;
    BlendCompositor* blendCompositor;
    Texture2D fftTexture;       // 1D texture (1025x1) for normalized FFT magnitudes
    float fftMaxMagnitude;      // Running max for auto-normalization
    // Temporaries for RenderPass callbacks
    float currentDeltaTime;
    float currentBlurScale;
    float currentKaleidoRotation;
    float currentKaleidoTime;
    float currentKaleidoFocal[2];
    float infiniteZoomTime;
    float sineWarpTime;
    float waveRippleTime;
    float currentWaveRippleOrigin[2];
    float mobiusTime;
    float currentMobiusPoint1[2];
    float currentMobiusPoint2[2];
    float glitchTime;
    int glitchFrame;
    float poincareTime;
    float currentPoincareTranslation[2];
    float currentPoincareRotation;
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect* PostEffectInit(int screenWidth, int screenHeight);

// Clean up post-effect resources
void PostEffectUninit(PostEffect* pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect* pe, int width, int height);

// Clear feedback buffers and reset simulations (call when switching presets)
void PostEffectClearFeedback(PostEffect* pe);

// Begin drawing waveforms to accumulation texture
void PostEffectBeginDrawStage(PostEffect* pe);

// End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

#endif // POST_EFFECT_H
