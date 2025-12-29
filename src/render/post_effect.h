#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include <stdint.h>
#include "raylib.h"
#include "config/effect_config.h"

typedef struct Physarum Physarum;

typedef struct PostEffect {
    RenderTexture2D accumTexture;     // Feedback buffer (persists between frames)
    RenderTexture2D pingPong[2];      // Ping-pong buffers for multi-pass effects
    RenderTexture2D shapeSampleTex;   // Copy of accumTexture for textured shapes to sample
    Shader feedbackShader;
    Shader blurHShader;
    Shader blurVShader;
    Shader chromaticShader;
    Shader kaleidoShader;
    Shader voronoiShader;
    Shader trailBoostShader;
    Shader fxaaShader;
    Shader gammaShader;
    Shader shapeTextureShader;
    int shapeTexZoomLoc;
    int shapeTexAngleLoc;
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
    int voronoiResolutionLoc;
    int voronoiScaleLoc;
    int voronoiIntensityLoc;
    int voronoiTimeLoc;
    int voronoiSpeedLoc;
    int voronoiEdgeWidthLoc;
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
    int trailMapLoc;
    int trailBoostIntensityLoc;
    int trailBlendModeLoc;
    int fxaaResolutionLoc;
    int gammaGammaLoc;
    EffectConfig effects;
    int screenWidth;
    int screenHeight;
    float voronoiTime;
    Physarum* physarum;
    Texture2D fftTexture;       // 1D texture (1025x1) for normalized FFT magnitudes
    float fftMaxMagnitude;      // Running max for auto-normalization
    // Temporaries for RenderPass callbacks
    float currentDeltaTime;
    float currentBlurScale;
    float currentKaleidoRotation;
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect* PostEffectInit(int screenWidth, int screenHeight);

// Clean up post-effect resources
void PostEffectUninit(PostEffect* pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect* pe, int width, int height);

// Begin drawing waveforms to accumulation texture
void PostEffectBeginDrawStage(PostEffect* pe);

// End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

#endif // POST_EFFECT_H
