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
    Shader mobiusShader;
    Shader turbulenceShader;
    Shader radialStreakShader;
    Shader multiInversionShader;
    Shader textureWarpShader;
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
    int infiniteZoomFocalLoc;
    int infiniteZoomLayersLoc;
    int infiniteZoomSpiralAngleLoc;
    int infiniteZoomSpiralTwistLoc;
    int infiniteZoomDrosteShearLoc;
    int mobiusTimeLoc;
    int mobiusIterationsLoc;
    int mobiusPoleMagLoc;
    int mobiusRotationLoc;
    int mobiusUvScaleLoc;
    int turbulenceTimeLoc;
    int turbulenceOctavesLoc;
    int turbulenceStrengthLoc;
    int turbulenceOctaveTwistLoc;
    int turbulenceUvScaleLoc;
    int radialStreakTimeLoc;
    int radialStreakSamplesLoc;
    int radialStreakStreakLengthLoc;
    int radialStreakSpiralTwistLoc;
    int radialStreakFocalLoc;
    int multiInversionTimeLoc;
    int multiInversionIterationsLoc;
    int multiInversionRadiusLoc;
    int multiInversionRadiusScaleLoc;
    int multiInversionFocalAmplitudeLoc;
    int multiInversionFocalFreqXLoc;
    int multiInversionFocalFreqYLoc;
    int multiInversionPhaseOffsetLoc;
    int textureWarpStrengthLoc;
    int textureWarpIterationsLoc;
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
    float infiniteZoomFocal[2];
    float mobiusTime;
    float turbulenceTime;
    float radialStreakTime;
    float radialStreakFocal[2];
    float multiInversionTime;
    float mobiusRotation;         // CPU-accumulated rotation phase for mobius transform
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
