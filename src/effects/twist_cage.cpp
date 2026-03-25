// Twist Cage effect module implementation
// Nested platonic solid wireframes with per-layer twist and FFT-reactive glow -
// raw vertex/edge data uploaded to shader, all rotation and projection done
// GPU-side

#include "twist_cage.h"
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
// Init / Setup / Uninit
// ---------------------------------------------------------------------------

static void CacheLocations(TwistCageEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->scaleRatioLoc = GetShaderLocation(e->shader, "scaleRatio");
  e->twistAngleLoc = GetShaderLocation(e->shader, "twistAngle");
  e->twistPhaseLoc = GetShaderLocation(e->shader, "twistPhase");
  e->twistPitchLoc = GetShaderLocation(e->shader, "twistPitch");
  e->twistPitchPhaseLoc = GetShaderLocation(e->shader, "twistPitchPhase");
  e->twistRollLoc = GetShaderLocation(e->shader, "twistRoll");
  e->twistRollPhaseLoc = GetShaderLocation(e->shader, "twistRollPhase");
  e->perspectiveLoc = GetShaderLocation(e->shader, "perspective");
  e->scaleLoc = GetShaderLocation(e->shader, "scale");
  e->cameraPitchLoc = GetShaderLocation(e->shader, "cameraPitch");
  e->cameraYawLoc = GetShaderLocation(e->shader, "cameraYaw");
  e->lineWidthLoc = GetShaderLocation(e->shader, "lineWidth");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->contrastLoc = GetShaderLocation(e->shader, "contrast");
  e->verticesLoc = GetShaderLocation(e->shader, "vertices");
  e->vertexCountLoc = GetShaderLocation(e->shader, "vertexCount");
  e->edgeIdxLoc = GetShaderLocation(e->shader, "edgeIdx");
  e->edgeCountLoc = GetShaderLocation(e->shader, "edgeCount");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
}

bool TwistCageEffectInit(TwistCageEffect *e, const TwistCageConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/twist_cage.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->twistPhase = 0.0f;
  e->twistPitchPhase = 0.0f;
  e->twistRollPhase = 0.0f;

  return true;
}

static void UploadUniforms(TwistCageEffect *e, const TwistCageConfig *cfg,
                           const ShapeDescriptor *shape, float cameraPitch,
                           float cameraYaw, const Texture2D &fftTexture) {
  float verts[MAX_VERTICES * 3] = {};
  for (int v = 0; v < shape->vertexCount; v++) {
    verts[v * 3 + 0] = shape->vertices[v][0];
    verts[v * 3 + 1] = shape->vertices[v][1];
    verts[v * 3 + 2] = shape->vertices[v][2];
  }
  float edgeIdx[MAX_EDGES * 2] = {};
  for (int ei = 0; ei < shape->edgeCount; ei++) {
    edgeIdx[ei * 2 + 0] = (float)shape->edges[ei][0];
    edgeIdx[ei * 2 + 1] = (float)shape->edges[ei][1];
  }

  const float resolution[2] = {(float)GetScreenWidth(),
                                (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueV(e->shader, e->verticesLoc, verts, SHADER_UNIFORM_VEC3,
                  shape->vertexCount);
  SetShaderValue(e->shader, e->vertexCountLoc, &shape->vertexCount,
                 SHADER_UNIFORM_INT);
  SetShaderValueV(e->shader, e->edgeIdxLoc, edgeIdx, SHADER_UNIFORM_VEC2,
                  shape->edgeCount);
  SetShaderValue(e->shader, e->edgeCountLoc, &shape->edgeCount,
                 SHADER_UNIFORM_INT);

  int layerCount = cfg->layerCount < 2 ? 2 : cfg->layerCount;
  SetShaderValue(e->shader, e->layerCountLoc, &layerCount, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scaleRatioLoc, &cfg->scaleRatio,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistAngleLoc, &cfg->twistAngle,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistPhaseLoc, &e->twistPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistPitchLoc, &cfg->twistPitch,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistPitchPhaseLoc, &e->twistPitchPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistRollLoc, &cfg->twistRoll,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->twistRollPhaseLoc, &e->twistRollPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->perspectiveLoc, &cfg->perspective,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scaleLoc, &cfg->scale, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraPitchLoc, &cameraPitch,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cameraYawLoc, &cameraYaw, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineWidthLoc, &cfg->lineWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->contrastLoc, &cfg->contrast,
                 SHADER_UNIFORM_FLOAT);

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

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void TwistCageEffectSetup(TwistCageEffect *e, TwistCageConfig *cfg,
                          float deltaTime, const Texture2D &fftTexture) {
  e->twistPhase += cfg->twistSpeed * deltaTime;
  e->twistPitchPhase += cfg->twistPitchSpeed * deltaTime;
  e->twistRollPhase += cfg->twistRollSpeed * deltaTime;

  float cameraPitch = 0.0f;
  float cameraYaw = 0.0f;
  DualLissajousUpdate(&cfg->lissajous, deltaTime, 0.0f, &cameraPitch,
                      &cameraYaw);

  int shapeIdx = cfg->shape;
  if (shapeIdx < 0) {
    shapeIdx = 0;
  }
  if (shapeIdx >= SHAPE_COUNT) {
    shapeIdx = SHAPE_COUNT - 1;
  }

  UploadUniforms(e, cfg, &SHAPES[shapeIdx], cameraPitch, cameraYaw, fftTexture);
}

void TwistCageEffectUninit(TwistCageEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void TwistCageRegisterParams(TwistCageConfig *cfg) {
  ModEngineRegisterParam("twistCage.scaleRatio", &cfg->scaleRatio, 0.5f,
                         0.99f);
  ModEngineRegisterParam("twistCage.twistAngle", &cfg->twistAngle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("twistCage.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("twistCage.twistPitch", &cfg->twistPitch,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("twistCage.twistPitchSpeed", &cfg->twistPitchSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("twistCage.twistRoll", &cfg->twistRoll,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("twistCage.twistRollSpeed", &cfg->twistRollSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("twistCage.perspective", &cfg->perspective, 1.0f,
                         20.0f);
  ModEngineRegisterParam("twistCage.scale", &cfg->scale, 0.1f, 5.0f);
  ModEngineRegisterParam("twistCage.lineWidth", &cfg->lineWidth, 0.5f, 10.0f);
  ModEngineRegisterParam("twistCage.glowIntensity", &cfg->glowIntensity, 0.1f,
                         5.0f);
  ModEngineRegisterParam("twistCage.contrast", &cfg->contrast, 0.5f, 10.0f);
  ModEngineRegisterParam("twistCage.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("twistCage.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("twistCage.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("twistCage.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("twistCage.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("twistCage.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("twistCage.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("twistCage.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupTwistCage(PostEffect *pe) {
  TwistCageEffectSetup(&pe->twistCage, &pe->effects.twistCage,
                       pe->currentDeltaTime, pe->fftTexture);
}

void SetupTwistCageBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.twistCage.blendIntensity,
                       pe->effects.twistCage.blendMode);
}

// === UI ===

static void DrawAudio(TwistCageConfig *cfg, const ModSources *modSources) {
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##twistCage", &cfg->baseFreq,
                    "twistCage.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##twistCage", &cfg->maxFreq,
                    "twistCage.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##twistCage", &cfg->gain, "twistCage.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##twistCage", &cfg->curve, "twistCage.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##twistCage", &cfg->baseBright,
                    "twistCage.baseBright", "%.2f", modSources);
}

static void DrawTwistCageParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  TwistCageConfig *cfg = &e->twistCage;

  ImGui::SeparatorText("Geometry");
  ImGui::Combo("Shape##twistCage", &cfg->shape,
               "Tetrahedron\0Cube\0Octahedron\0Dodecahedron\0Icosahedron\0");
  ImGui::SliderInt("Layers##twistCage", &cfg->layerCount, 2, 20);

  ImGui::SeparatorText("Twist");
  ModulatableSliderAngleDeg("Yaw Angle##twistCage", &cfg->twistAngle,
                            "twistCage.twistAngle", modSources);
  ModulatableSliderSpeedDeg("Yaw Speed##twistCage", &cfg->twistSpeed,
                            "twistCage.twistSpeed", modSources);
  ModulatableSliderAngleDeg("Pitch Angle##twistCage", &cfg->twistPitch,
                            "twistCage.twistPitch", modSources);
  ModulatableSliderSpeedDeg("Pitch Speed##twistCage", &cfg->twistPitchSpeed,
                            "twistCage.twistPitchSpeed", modSources);
  ModulatableSliderAngleDeg("Roll Angle##twistCage", &cfg->twistRoll,
                            "twistCage.twistRoll", modSources);
  ModulatableSliderSpeedDeg("Roll Speed##twistCage", &cfg->twistRollSpeed,
                            "twistCage.twistRollSpeed", modSources);

  ImGui::SeparatorText("Projection");
  ModulatableSlider("Perspective##twistCage", &cfg->perspective,
                    "twistCage.perspective", "%.1f", modSources);
  ModulatableSlider("Scale##twistCage", &cfg->scale, "twistCage.scale",
                    "%.2f", modSources);

  ImGui::SeparatorText("Glow");
  ModulatableSlider("Line Width##twistCage", &cfg->lineWidth,
                    "twistCage.lineWidth", "%.1f", modSources);
  ModulatableSlider("Glow Intensity##twistCage", &cfg->glowIntensity,
                    "twistCage.glowIntensity", "%.1f", modSources);
  ModulatableSlider("Contrast##twistCage_glow", &cfg->contrast,
                    "twistCage.contrast", "%.1f", modSources);

  ImGui::SeparatorText("Camera");
  DrawLissajousControls(&cfg->lissajous, "twistCage", "twistCage.lissajous",
                        modSources);

  DrawAudio(cfg, modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(twistCage)
REGISTER_GENERATOR(TRANSFORM_TWIST_CAGE_BLEND, TwistCage, twistCage,
                   "Twist Cage", SetupTwistCageBlend, SetupTwistCage, 10,
                   DrawTwistCageParams, DrawOutput_twistCage)
// clang-format on
