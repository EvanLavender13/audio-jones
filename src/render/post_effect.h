#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include <stdint.h>
#include "raylib.h"
#include "config/effect_config.h"
#include "automation/lfo.h"

typedef struct Physarum Physarum;

typedef struct PostEffect {
    RenderTexture2D accumTexture;     // Feedback buffer (persists between frames)
    RenderTexture2D tempTexture;      // Ping-pong buffer for multi-pass effects
    RenderTexture2D kaleidoTexture;   // Dedicated kaleidoscope output (non-feedback)
    Shader feedbackShader;
    Shader blurHShader;
    Shader blurVShader;
    Shader chromaticShader;
    Shader kaleidoShader;
    Shader voronoiShader;
    Shader trailBoostShader;
    Shader fxaaShader;
    Shader gammaShader;
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
    int feedbackZoomLoc;
    int feedbackRotationLoc;
    int feedbackDesaturateLoc;
    int warpStrengthLoc;
    int warpScaleLoc;
    int warpOctavesLoc;
    int warpLacunarityLoc;
    int warpTimeLoc;
    int trailMapLoc;
    int trailBoostIntensityLoc;
    int trailBlendModeLoc;
    int fxaaResolutionLoc;
    int gammaGammaLoc;
    float currentBeatIntensity;
    EffectConfig effects;
    int screenWidth;
    int screenHeight;
    LFOState rotationLFOState;
    float voronoiTime;
    float warpTime;
    Physarum* physarum;
    Texture2D fftTexture;       // 1D texture (1025x1) for normalized FFT magnitudes
    float fftMaxMagnitude;      // Running max for auto-normalization
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect* PostEffectInit(int screenWidth, int screenHeight);

// Clean up post-effect resources
void PostEffectUninit(PostEffect* pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect* pe, int width, int height);

// STAGE 1: Apply feedback/warp effects (physarum, voronoi, feedback shader, blur)
// Processes the feedback buffer, closes texture mode when done
void PostEffectApplyFeedbackStage(PostEffect* pe, float deltaTime, float beatIntensity,
                                   const float* fftMagnitude);

// STAGE 2: Begin drawing waveforms to accumulation texture
void PostEffectBeginDrawStage(PostEffect* pe);

// STAGE 2: End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

// Draw accumulated texture to screen
// globalTick: shared counter for kaleidoscope rotation
void PostEffectToScreen(PostEffect* pe, uint64_t globalTick);

#endif // POST_EFFECT_H
