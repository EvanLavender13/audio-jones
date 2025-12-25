#ifndef EXPERIMENTAL_EFFECT_H
#define EXPERIMENTAL_EFFECT_H

#include "raylib.h"
#include "config/experimental_config.h"

typedef struct ExperimentalEffect {
    RenderTexture2D expAccumTexture;    // Main feedback accumulation buffer
    RenderTexture2D expTempTexture;     // Ping-pong buffer for feedback processing
    RenderTexture2D injectionTexture;   // Waveform injection buffer (drawn at low opacity)
    Shader feedbackExpShader;           // Blur + decay + zoom shader
    Shader blendInjectShader;           // Blends injection into feedback
    Shader compositeShader;             // Display-only post-processing (gamma, etc.)
    int feedbackResolutionLoc;
    int feedbackHalfLifeLoc;
    int feedbackDeltaTimeLoc;
    int feedbackZoomBaseLoc;
    int feedbackZoomRadialLoc;
    int feedbackRotBaseLoc;
    int feedbackRotRadialLoc;
    int feedbackDxBaseLoc;
    int feedbackDxRadialLoc;
    int feedbackDyBaseLoc;
    int feedbackDyRadialLoc;
    int blendInjectionTexLoc;
    int blendInjectionOpacityLoc;
    int compositeGammaLoc;
    int screenWidth;
    int screenHeight;
    ExperimentalConfig config;
} ExperimentalEffect;

// Initialize experimental effect processor with screen dimensions
// Loads shaders and creates render textures
// Returns NULL on failure
ExperimentalEffect* ExperimentalEffectInit(int screenWidth, int screenHeight);

// Clean up experimental effect resources
void ExperimentalEffectUninit(ExperimentalEffect* exp);

// Resize render textures (call when window resizes)
void ExperimentalEffectResize(ExperimentalEffect* exp, int width, int height);

// Begin rendering to injection texture
// Applies feedback shader to accumulation, then opens injection texture for waveform drawing
// deltaTime: frame time in seconds for framerate-independent decay
void ExperimentalEffectBeginAccum(ExperimentalEffect* exp, float deltaTime);

// End rendering to injection texture
// Blends injection into accumulation at configured opacity
void ExperimentalEffectEndAccum(ExperimentalEffect* exp);

// Draw accumulated texture to screen as fullscreen quad
void ExperimentalEffectToScreen(ExperimentalEffect* exp);

// Clear all textures to black (call when switching pipelines)
void ExperimentalEffectClear(ExperimentalEffect* exp);

#endif // EXPERIMENTAL_EFFECT_H
