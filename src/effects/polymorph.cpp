// Polymorph effect module implementation
// SDF ray-marched glowing wireframe polyhedron that morphs between platonic
// solids -- CPU-side morph state machine computes per-edge capsule endpoints,
// shader ray-marches capsule SDF with FFT-reactive glow

#include "polymorph.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "config/platonic_solids.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// PRNG helpers
// ---------------------------------------------------------------------------

static uint32_t Xorshift32(uint32_t *state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

static float PrngFloat(uint32_t *state) {
  return (float)(Xorshift32(state) & 0xFFFFFF) / (float)0xFFFFFF;
}

static float PrngFloatSigned(uint32_t *state) {
  return PrngFloat(state) * 2.0f - 1.0f;
}

// ---------------------------------------------------------------------------
// Init / Setup / Uninit
// ---------------------------------------------------------------------------

bool PolymorphEffectInit(PolymorphEffect *e, const PolymorphConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/polymorph.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->edgeALoc = GetShaderLocation(e->shader, "edgeA");
  e->edgeBLoc = GetShaderLocation(e->shader, "edgeB");
  e->edgeTLoc = GetShaderLocation(e->shader, "edgeT");
  e->edgeCountLoc = GetShaderLocation(e->shader, "edgeCount");
  e->edgeThicknessLoc = GetShaderLocation(e->shader, "edgeThickness");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->cameraOriginLoc = GetShaderLocation(e->shader, "cameraOrigin");
  e->cameraFovLoc = GetShaderLocation(e->shader, "cameraFov");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->morphPhase = 0.0f;
  e->currentShape = cfg->baseShape;
  e->orbitAccum = 0.0f;
  e->rngState = 0xDEADBEEF;
  e->edgeCount = 0;

  for (int v = 0; v < MAX_VERTICES; v++) {
    e->vertexOffsetX[v] = 0.0f;
    e->vertexOffsetY[v] = 0.0f;
    e->vertexOffsetZ[v] = 0.0f;
  }

  return true;
}

// Regenerate freeform vertex offsets via PRNG
static void RegenerateFreeformOffsets(PolymorphEffect *e) {
  for (int v = 0; v < MAX_VERTICES; v++) {
    e->vertexOffsetX[v] = PrngFloatSigned(&e->rngState);
    e->vertexOffsetY[v] = PrngFloatSigned(&e->rngState);
    e->vertexOffsetZ[v] = PrngFloatSigned(&e->rngState);
  }
}

// Pick next shape based on randomness parameter
static void SelectNextShape(PolymorphEffect *e, const PolymorphConfig *cfg) {
  const float roll = PrngFloat(&e->rngState);
  if (roll < cfg->randomness) {
    e->currentShape = (int)(Xorshift32(&e->rngState) % 5);
  } else {
    e->currentShape = cfg->baseShape;
  }
}

// Upload all shader uniforms for the current frame
static void UploadUniforms(PolymorphEffect *e, const PolymorphConfig *cfg,
                           float cameraOrigin[3], Texture2D fftTexture) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Pack edge A endpoints into interleaved vec3 array
  float edgeAData[90];
  float edgeBData[90];
  for (int i = 0; i < e->edgeCount; i++) {
    edgeAData[i * 3 + 0] = e->edgeAx[i];
    edgeAData[i * 3 + 1] = e->edgeAy[i];
    edgeAData[i * 3 + 2] = e->edgeAz[i];
    edgeBData[i * 3 + 0] = e->edgeBx[i];
    edgeBData[i * 3 + 1] = e->edgeBy[i];
    edgeBData[i * 3 + 2] = e->edgeBz[i];
  }

  SetShaderValueV(e->shader, e->edgeALoc, edgeAData, SHADER_UNIFORM_VEC3,
                  e->edgeCount);
  SetShaderValueV(e->shader, e->edgeBLoc, edgeBData, SHADER_UNIFORM_VEC3,
                  e->edgeCount);
  SetShaderValueV(e->shader, e->edgeTLoc, e->edgeT, SHADER_UNIFORM_FLOAT,
                  e->edgeCount);
  SetShaderValue(e->shader, e->edgeCountLoc, &e->edgeCount, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->edgeThicknessLoc, &cfg->edgeThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraOriginLoc, cameraOrigin,
                 SHADER_UNIFORM_VEC3);
  SetShaderValue(e->shader, e->cameraFovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

// Compute perturbed vertex positions on unit sphere, scaled
static void ComputePerturbedVertices(const ShapeDescriptor *shape,
                                     const PolymorphEffect *e,
                                     const PolymorphConfig *cfg,
                                     float verts[][3]) {
  for (int v = 0; v < shape->vertexCount; v++) {
    const float px =
        shape->vertices[v][0] + cfg->freeform * e->vertexOffsetX[v];
    const float py =
        shape->vertices[v][1] + cfg->freeform * e->vertexOffsetY[v];
    const float pz =
        shape->vertices[v][2] + cfg->freeform * e->vertexOffsetZ[v];

    const float len = sqrtf(px * px + py * py + pz * pz);
    if (len > 0.0f) {
      const float invLen = cfg->scale / len;
      verts[v][0] = px * invLen;
      verts[v][1] = py * invLen;
      verts[v][2] = pz * invLen;
    } else {
      verts[v][0] = 0.0f;
      verts[v][1] = 0.0f;
      verts[v][2] = 0.0f;
    }
  }
}

// Compute per-edge animated capsule endpoints based on morph phase
static void ComputeEdgeCapsules(PolymorphEffect *e,
                                const ShapeDescriptor *shape,
                                const float verts[][3], float slideT,
                                float expandEnd, float collapseStart) {
  e->edgeCount = shape->edgeCount;
  for (int ei = 0; ei < shape->edgeCount; ei++) {
    const int i = shape->edges[ei][0];
    const int j = shape->edges[ei][1];

    if (e->morphPhase < expandEnd) {
      // EXPANDING: edge grows from point at j toward i
      e->edgeAx[ei] = verts[j][0] + slideT * (verts[i][0] - verts[j][0]);
      e->edgeAy[ei] = verts[j][1] + slideT * (verts[i][1] - verts[j][1]);
      e->edgeAz[ei] = verts[j][2] + slideT * (verts[i][2] - verts[j][2]);
      e->edgeBx[ei] = verts[j][0];
      e->edgeBy[ei] = verts[j][1];
      e->edgeBz[ei] = verts[j][2];
    } else if (e->morphPhase < collapseStart) {
      // HOLDING: full edge
      e->edgeAx[ei] = verts[i][0];
      e->edgeAy[ei] = verts[i][1];
      e->edgeAz[ei] = verts[i][2];
      e->edgeBx[ei] = verts[j][0];
      e->edgeBy[ei] = verts[j][1];
      e->edgeBz[ei] = verts[j][2];
    } else {
      // COLLAPSING: B slides from v[j] toward v[i] as slideT goes 1->0
      e->edgeAx[ei] = verts[i][0];
      e->edgeAy[ei] = verts[i][1];
      e->edgeAz[ei] = verts[i][2];
      e->edgeBx[ei] = verts[i][0] + slideT * (verts[j][0] - verts[i][0]);
      e->edgeBy[ei] = verts[i][1] + slideT * (verts[j][1] - verts[i][1]);
      e->edgeBz[ei] = verts[i][2] + slideT * (verts[j][2] - verts[i][2]);
    }

    e->edgeT[ei] =
        (e->edgeCount > 1) ? (float)ei / (float)(e->edgeCount - 1) : 0.5f;
  }
}

void PolymorphEffectSetup(PolymorphEffect *e, const PolymorphConfig *cfg,
                          float deltaTime, Texture2D fftTexture) {
  e->morphPhase += cfg->morphSpeed * deltaTime;
  if (e->morphPhase >= 1.0f) {
    e->morphPhase -= 1.0f;
    SelectNextShape(e, cfg);
    RegenerateFreeformOffsets(e);
  }

  const float expandEnd = (1.0f - cfg->holdRatio) / 2.0f;
  const float collapseStart = expandEnd + cfg->holdRatio;

  float slideT;
  if (e->morphPhase < expandEnd) {
    slideT = (expandEnd > 0.0f) ? e->morphPhase / expandEnd : 1.0f;
  } else if (e->morphPhase < collapseStart) {
    slideT = 1.0f;
  } else {
    const float collapseDuration = 1.0f - collapseStart;
    slideT = (collapseDuration > 0.0f)
                 ? 1.0f - (e->morphPhase - collapseStart) / collapseDuration
                 : 0.0f;
  }

  int shapeIdx = e->currentShape;
  if (shapeIdx < 0) {
    shapeIdx = 0;
  }
  if (shapeIdx >= SHAPE_COUNT) {
    shapeIdx = SHAPE_COUNT - 1;
  }
  const ShapeDescriptor *shape = &SHAPES[shapeIdx];

  float verts[MAX_VERTICES][3];
  ComputePerturbedVertices(shape, e, cfg, verts);
  ComputeEdgeCapsules(e, shape, verts, slideT, expandEnd, collapseStart);

  e->orbitAccum += cfg->orbitSpeed * deltaTime;
  float cameraOrigin[3] = {
      cfg->cameraDistance * sinf(e->orbitAccum),
      cfg->cameraHeight,
      cfg->cameraDistance * cosf(e->orbitAccum),
  };

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  UploadUniforms(e, cfg, cameraOrigin, fftTexture);
}

void PolymorphEffectUninit(PolymorphEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void PolymorphRegisterParams(PolymorphConfig *cfg) {
  ModEngineRegisterParam("polymorph.randomness", &cfg->randomness, 0.0f, 1.0f);
  ModEngineRegisterParam("polymorph.freeform", &cfg->freeform, 0.0f, 1.0f);
  ModEngineRegisterParam("polymorph.morphSpeed", &cfg->morphSpeed, 0.1f, 5.0f);
  ModEngineRegisterParam("polymorph.holdRatio", &cfg->holdRatio, 0.0f, 1.0f);
  ModEngineRegisterParam("polymorph.orbitSpeed", &cfg->orbitSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("polymorph.cameraHeight", &cfg->cameraHeight, 0.0f,
                         15.0f);
  ModEngineRegisterParam("polymorph.cameraDistance", &cfg->cameraDistance, 5.0f,
                         50.0f);
  ModEngineRegisterParam("polymorph.scale", &cfg->scale, 1.0f, 30.0f);
  ModEngineRegisterParam("polymorph.edgeThickness", &cfg->edgeThickness, 0.005f,
                         0.1f);
  ModEngineRegisterParam("polymorph.glowIntensity", &cfg->glowIntensity, 0.05f,
                         1.0f);
  ModEngineRegisterParam("polymorph.fov", &cfg->fov, 1.0f, 4.0f);
  ModEngineRegisterParam("polymorph.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("polymorph.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("polymorph.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("polymorph.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("polymorph.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("polymorph.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupPolymorph(PostEffect *pe) {
  PolymorphEffectSetup(&pe->polymorph, &pe->effects.polymorph,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupPolymorphBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.polymorph.blendIntensity,
                       pe->effects.polymorph.blendMode);
}

// === UI ===

static void DrawPolymorphParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  PolymorphConfig *cfg = &e->polymorph;

  // Shape
  ImGui::SeparatorText("Shape");
  ImGui::Combo("Shape##polymorph", &cfg->baseShape,
               "Tetrahedron\0Cube\0Octahedron\0Dodecahedron\0Icosahedron\0");
  ModulatableSlider("Randomness##polymorph", &cfg->randomness,
                    "polymorph.randomness", "%.2f", modSources);
  ModulatableSlider("Freeform##polymorph", &cfg->freeform, "polymorph.freeform",
                    "%.2f", modSources);

  // Morph
  ImGui::SeparatorText("Morph");
  ModulatableSlider("Morph Speed##polymorph", &cfg->morphSpeed,
                    "polymorph.morphSpeed", "%.2f", modSources);
  ModulatableSlider("Hold##polymorph", &cfg->holdRatio, "polymorph.holdRatio",
                    "%.2f", modSources);

  // Camera
  ImGui::SeparatorText("Camera");
  ModulatableSliderSpeedDeg("Orbit Speed##polymorph", &cfg->orbitSpeed,
                            "polymorph.orbitSpeed", modSources);
  ModulatableSlider("Camera Height##polymorph", &cfg->cameraHeight,
                    "polymorph.cameraHeight", "%.1f", modSources);
  ModulatableSlider("Camera Dist##polymorph", &cfg->cameraDistance,
                    "polymorph.cameraDistance", "%.1f", modSources);
  ModulatableSlider("FOV##polymorph", &cfg->fov, "polymorph.fov", "%.1f",
                    modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Scale##polymorph", &cfg->scale, "polymorph.scale", "%.1f",
                    modSources);
  ModulatableSliderLog("Edge Thickness##polymorph", &cfg->edgeThickness,
                       "polymorph.edgeThickness", "%.3f", modSources);
  ModulatableSlider("Glow##polymorph", &cfg->glowIntensity,
                    "polymorph.glowIntensity", "%.2f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##polymorph", &cfg->baseFreq,
                    "polymorph.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##polymorph", &cfg->maxFreq,
                    "polymorph.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##polymorph", &cfg->gain, "polymorph.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##polymorph", &cfg->curve, "polymorph.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##polymorph", &cfg->baseBright,
                    "polymorph.baseBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(polymorph)
REGISTER_GENERATOR(TRANSFORM_POLYMORPH_BLEND, Polymorph, polymorph,
                   "Polymorph", SetupPolymorphBlend, SetupPolymorph, 10,
                   DrawPolymorphParams, DrawOutput_polymorph)
// clang-format on
