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

// Begin rendering to accumulation texture
// Call this before drawing waveforms
void VisualizerBeginAccum(Visualizer* vis);

// End rendering to accumulation texture
// Call this after drawing waveforms
void VisualizerEndAccum(Visualizer* vis);

// Draw accumulated texture to screen
void VisualizerToScreen(Visualizer* vis);

#endif // VISUALIZER_H
