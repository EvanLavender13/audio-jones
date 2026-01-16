#ifndef CYMATICS_H
#define CYMATICS_H

#include <stdbool.h>
#include "raylib.h"
#include "render/blend_mode.h"
#include "render/color_config.h"

typedef struct TrailMap TrailMap;
typedef struct ColorLUT ColorLUT;

typedef struct CymaticsConfig {
    bool enabled = false;
    float waveScale = 10.0f;           // Pattern scale - higher = larger (1-50)
    float falloff = 1.0f;              // Distance attenuation (0-5)
    float visualGain = 2.0f;           // Output intensity (0.5-5)
    int contourCount = 0;              // Banding (0=smooth, 1-10)
    float decayHalfLife = 0.5f;        // Trail persistence (0.1-5)
    int diffusionScale = 1;            // Blur kernel size (0-4)
    float boostIntensity = 1.0f;       // Trail boost strength (0-5)
    int sourceCount = 5;               // Number of sources (1-8)
    float sourceAmplitude = 0.2f;      // Lissajous motion amplitude (0.0-0.5)
    float sourceFreqX = 0.05f;         // Lissajous X frequency (Hz)
    float sourceFreqY = 0.08f;         // Lissajous Y frequency (Hz)
    float baseRadius = 0.4f;           // Base position distance from center (0.0-0.5)
    float patternAngle = 0.0f;         // Pattern rotation offset (radians)
    EffectBlendMode blendMode = EFFECT_BLEND_BOOST;
    bool debugOverlay = false;
    ColorConfig color;
} CymaticsConfig;

typedef struct Cymatics {
    unsigned int computeProgram;
    TrailMap* trailMap;
    ColorLUT* colorLUT;
    Shader debugShader;
    int width;
    int height;

    // Uniform locations
    int resolutionLoc;
    int waveScaleLoc;
    int falloffLoc;
    int visualGainLoc;
    int contourCountLoc;
    int bufferSizeLoc;
    int writeIndexLoc;
    int valueLoc;
    int sourcesLoc;
    int sourceCountLoc;

    float sourcePhase;  // Lissajous phase accumulator (radians)
    CymaticsConfig config;
    bool supported;
} Cymatics;

// Check if compute shaders are supported (OpenGL 4.3+)
bool CymaticsSupported(void);

// Initialize cymatics simulation
// Returns NULL if compute shaders not supported or allocation fails
Cymatics* CymaticsInit(int width, int height, const CymaticsConfig* config);

// Clean up cymatics resources
void CymaticsUninit(Cymatics* cym);

// Dispatch compute shader to update simulation
void CymaticsUpdate(Cymatics* cym, Texture2D waveformTexture, int writeIndex, float deltaTime);

// Process trails with diffusion and decay
void CymaticsProcessTrails(Cymatics* cym, float deltaTime);

// Draw debug overlay (trail visualization)
void CymaticsDrawDebug(Cymatics* cym);

// Update dimensions
void CymaticsResize(Cymatics* cym, int width, int height);

// Clear trails
void CymaticsReset(Cymatics* cym);

// Apply config changes
void CymaticsApplyConfig(Cymatics* cym, const CymaticsConfig* newConfig);

#endif // CYMATICS_H
