#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include <stdint.h>

typedef struct PostEffect PostEffect;

// Apply feedback stage effects (voronoi, feedback, blur)
// Updates accumTexture with processed frame
void RenderPipelineApplyFeedback(PostEffect* pe, float deltaTime, const float* fftMagnitude);

// Copy accumTexture to shapeSampleTex for textured shapes to read
// Call after feedback pass, before post-feedback drawable rendering
void RenderPipelineUpdateShapeSample(PostEffect* pe);

// Apply output stage effects and draw to screen
// Applies trail boost, kaleidoscope, chromatic, FXAA, gamma
void RenderPipelineApplyOutput(PostEffect* pe, uint64_t globalTick);

#endif // RENDER_PIPELINE_H
