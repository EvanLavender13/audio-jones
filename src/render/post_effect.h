#ifndef POST_EFFECT_H
#define POST_EFFECT_H

#include "config/effect_config.h"
#include "raylib.h"
#include <stdint.h>

typedef struct Physarum Physarum;
typedef struct CurlFlow CurlFlow;
typedef struct AttractorFlow AttractorFlow;
typedef struct ParticleLife ParticleLife;
typedef struct Boids Boids;
typedef struct MazeWorms MazeWorms;
typedef struct BlendCompositor BlendCompositor;
typedef struct ColorLUT ColorLUT;

// Progress callback type - called between init phases
// progress: 0.0 to 1.0
typedef void (*PostEffectProgressFn)(float progress, void *userData);

typedef struct PostEffect {
  RenderTexture2D accumTexture; // Feedback buffer (persists between frames)
  RenderTexture2D pingPong[2];  // Ping-pong buffers for multi-pass effects
  RenderTexture2D
      outputTexture; // Feedback-processed content for textured shape sampling
  Shader feedbackShader;
  Shader blurHShader;
  Shader blurVShader;
  Shader fxaaShader;
  Shader clarityShader;
  Shader gammaShader;
  Shader shapeTextureShader;
  RenderTexture2D halfResA;
  RenderTexture2D halfResB;
  int shapeTexZoomLoc;
  int shapeTexAngleLoc;
  int shapeTexBrightnessLoc;
  int blurHResolutionLoc;
  int blurVResolutionLoc;
  int blurHScaleLoc;
  int blurVScaleLoc;
  int halfLifeLoc;
  int deltaTimeLoc;
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
  int feedbackFlowStrengthLoc;
  int feedbackFlowAngleLoc;
  int feedbackFlowScaleLoc;
  int feedbackFlowThresholdLoc;
  int feedbackCxLoc;
  int feedbackCyLoc;
  int feedbackSxLoc;
  int feedbackSyLoc;
  int feedbackZoomAngularLoc;
  int feedbackZoomAngularFreqLoc;
  int feedbackRotAngularLoc;
  int feedbackRotAngularFreqLoc;
  int feedbackDxAngularLoc;
  int feedbackDxAngularFreqLoc;
  int feedbackDyAngularLoc;
  int feedbackDyAngularFreqLoc;
  int feedbackWarpLoc;
  int feedbackWarpTimeLoc;
  int feedbackWarpScaleInverseLoc;
  int fxaaResolutionLoc;
  int clarityResolutionLoc;
  int clarityAmountLoc;
  int gammaGammaLoc;
  EffectConfig effects;
  int screenWidth;
  int screenHeight;
  Physarum *physarum;
  CurlFlow *curlFlow;
  AttractorFlow *attractorFlow;
  ParticleLife *particleLife;
  Boids *boids;
  MazeWorms *mazeWorms;
  void *effectStates[TRANSFORM_EFFECT_COUNT];
  BlendCompositor *blendCompositor;
  RenderTexture2D
      generatorScratch;  // Shared scratch texture for generator blend rendering
  Texture2D fftTexture;  // 1D texture (1025x1) for normalized FFT magnitudes
  float fftMaxMagnitude; // Running max for auto-normalization
  Texture2D
      waveformTexture; // 1D texture (2048x1) for waveform history ring buffer
  int waveformWriteIndex;
  // Temporaries for RenderPass callbacks
  float currentDeltaTime;
  float currentBlurScale;
  float transformTime; // Shared animation time for transform effects
  float warpTime;
  Texture2D currentSceneTexture;
  RenderTexture2D
      *currentRenderDest; // Pipeline output for custom render effects
} PostEffect;

// Initialize post-effect processor with screen dimensions
// Loads shaders and creates accumulation texture
// Returns NULL on failure
PostEffect *PostEffectInit(int screenWidth, int screenHeight,
                           PostEffectProgressFn onProgress, void *userData);

// Clean up post-effect resources
void PostEffectUninit(PostEffect *pe);

// Resize render textures (call when window resizes)
void PostEffectResize(PostEffect *pe, int width, int height);

// Register per-effect params with the modulation engine
// Called after ParamRegistryInit; individual effects add their params here
void PostEffectRegisterParams(PostEffect *pe);

// Clear feedback buffers and reset simulations (call when switching presets)
void PostEffectClearFeedback(PostEffect *pe);

// Begin drawing waveforms to accumulation texture
void PostEffectBeginDrawStage(const PostEffect *pe);

// End drawing waveforms to accumulation texture
void PostEffectEndDrawStage(void);

#endif // POST_EFFECT_H
