#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include "raylib.h"
#include "config/effect_config.h"
#include "automation/lfo.h"

typedef struct PostEffect {
    RenderTexture2D accumTexture;
    RenderTexture2D tempTexture;
    Shader feedbackShader;
    Shader blurHShader;
    Shader blurVShader;
    Shader chromaticShader;
    int blurHResolutionLoc;
    int blurVResolutionLoc;
    int blurHScaleLoc;
    int blurVScaleLoc;
    int halfLifeLoc;
    int deltaTimeLoc;
    int chromaticResolutionLoc;
    int chromaticOffsetLoc;
    int feedbackZoomLoc;
    int feedbackRotationLoc;
    int feedbackDesaturateLoc;
    float currentBeatIntensity;
    EffectConfig effects;
    int screenWidth;
    int screenHeight;
    LFOState rotationLFOState;
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect* PostEffectInit(int screenWidth, int screenHeight);

// Clean up post-effect resources
void PostEffectUninit(PostEffect* pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect* pe, int width, int height);

// Begin rendering to accumulation texture
// Call this before drawing waveforms
// deltaTime: frame time in seconds for framerate-independent fade
// beatIntensity: 0.0-1.0 beat intensity for bloom pulse effect
void PostEffectBeginAccum(PostEffect* pe, float deltaTime, float beatIntensity);

// End rendering to accumulation texture
// Call this after drawing waveforms
void PostEffectEndAccum(PostEffect* pe);

// Draw accumulated texture to screen
void PostEffectToScreen(PostEffect* pe);

#endif // POST_EFFECT_H
