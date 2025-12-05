#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "raylib.h"

typedef struct Visualizer {
    RenderTexture2D accumTexture;
    RenderTexture2D tempTexture;
    Shader blurHShader;
    Shader blurVShader;
    int blurHResolutionLoc;
    int blurVResolutionLoc;
    int halfLifeLoc;
    int deltaTimeLoc;
    float halfLife;
    int screenWidth;
    int screenHeight;
} Visualizer;

// Initialize visualizer with screen dimensions
// Loads fade shader and creates accumulation texture
// Returns NULL on failure
Visualizer* VisualizerInit(int screenWidth, int screenHeight);

// Clean up visualizer resources
void VisualizerUninit(Visualizer* vis);

// Resize visualizer render textures (call when window resizes)
void VisualizerResize(Visualizer* vis, int width, int height);

// Begin rendering to accumulation texture
// Call this before drawing waveforms
// deltaTime: frame time in seconds for framerate-independent fade
void VisualizerBeginAccum(Visualizer* vis, float deltaTime);

// End rendering to accumulation texture
// Call this after drawing waveforms
void VisualizerEndAccum(Visualizer* vis);

// Draw accumulated texture to screen
void VisualizerToScreen(Visualizer* vis);

#endif // VISUALIZER_H
