#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "raylib.h"

typedef struct Visualizer Visualizer;

// Initialize visualizer with screen dimensions
// Loads fade shader and creates accumulation texture
// Returns NULL on failure
Visualizer* VisualizerInit(int screenWidth, int screenHeight);

// Clean up visualizer resources
void VisualizerUninit(Visualizer* vis);

// Resize visualizer render textures (call when window resizes)
void VisualizerResize(Visualizer* vis, int width, int height);

// Get current dimensions
int VisualizerGetWidth(Visualizer* vis);
int VisualizerGetHeight(Visualizer* vis);

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
