// Synapse Tree effect module implementation
// Raymarched sphere-inversion fractal tree with synapse pulses and trail
// persistence

#include "synapse_tree.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "render/render_utils.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static void InitPingPong(SynapseTreeEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "SYNAPSE_TREE");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "SYNAPSE_TREE");
}

static void UnloadPingPong(SynapseTreeEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

bool SynapseTreeEffectInit(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                           int width, int height) {
  e->shader = LoadShader(NULL, "shaders/synapse_tree.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->animPhaseLoc = GetShaderLocation(e->shader, "animPhase");
  e->marchStepsLoc = GetShaderLocation(e->shader, "marchSteps");
  e->foldIterationsLoc = GetShaderLocation(e->shader, "foldIterations");
  e->fovLoc = GetShaderLocation(e->shader, "fov");
  e->foldOffsetLoc = GetShaderLocation(e->shader, "foldOffset");
  e->yFoldLoc = GetShaderLocation(e->shader, "yFold");
  e->branchThicknessLoc = GetShaderLocation(e->shader, "branchThickness");
  e->camZLoc = GetShaderLocation(e->shader, "camZ");
  e->synapseIntensityLoc = GetShaderLocation(e->shader, "synapseIntensity");
  e->synapseBounceFreqLoc = GetShaderLocation(e->shader, "synapseBounceFreq");
  e->synapsePulseFreqLoc = GetShaderLocation(e->shader, "synapsePulseFreq");
  e->colorPhaseLoc = GetShaderLocation(e->shader, "colorPhase");
  e->colorStretchLoc = GetShaderLocation(e->shader, "colorStretch");
  e->brightnessLoc = GetShaderLocation(e->shader, "brightness");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->previousFrameLoc = GetShaderLocation(e->shader, "previousFrame");
  e->decayFactorLoc = GetShaderLocation(e->shader, "decayFactor");
  e->trailBlurLoc = GetShaderLocation(e->shader, "trailBlur");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  InitPingPong(e, width, height);
  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);
  e->readIdx = 0;
  e->animPhase = 0.0f;
  e->camZ = 0.0f;
  e->colorPhase = 0.0f;

  return true;
}

void SynapseTreeEffectSetup(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                            float deltaTime, Texture2D fftTexture) {
  e->animPhase += cfg->animSpeed * deltaTime;
  e->camZ = fmodf(e->camZ + cfg->driftSpeed * deltaTime, 100.0f);
  e->colorPhase += cfg->colorSpeed * deltaTime;
  e->currentFFTTexture = fftTexture;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->animPhaseLoc, &e->animPhase,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValue(e->shader, e->marchStepsLoc, &cfg->marchSteps,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->foldIterationsLoc, &cfg->foldIterations,
                 SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->fovLoc, &cfg->fov, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->foldOffsetLoc, &cfg->foldOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->yFoldLoc, &cfg->yFold, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->branchThicknessLoc, &cfg->branchThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->camZLoc, &e->camZ, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->synapseIntensityLoc, &cfg->synapseIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->synapseBounceFreqLoc, &cfg->synapseBounceFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->synapsePulseFreqLoc, &cfg->synapsePulseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseLoc, &e->colorPhase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorStretchLoc, &cfg->colorStretch,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->brightnessLoc, &cfg->brightness,
                 SHADER_UNIFORM_FLOAT);

  const float safeHalfLife = fmaxf(cfg->decayHalfLife, 0.001f);
  float decayFactor = expf(-0.693147f * deltaTime / safeHalfLife);
  SetShaderValue(e->shader, e->decayFactorLoc, &decayFactor,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->trailBlurLoc, &cfg->trailBlur,
                 SHADER_UNIFORM_FLOAT);

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
}

void SynapseTreeEffectRender(SynapseTreeEffect *e, const SynapseTreeConfig *cfg,
                             float deltaTime, int screenWidth,
                             int screenHeight) {
  (void)cfg;
  (void)deltaTime;

  const int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  BeginShaderMode(e->shader);

  SetShaderValueTexture(e->shader, e->previousFrameLoc,
                        e->pingPong[e->readIdx].texture);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValueTexture(e->shader, e->fftTextureLoc, e->currentFFTTexture);

  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture, screenWidth,
                                screenHeight);
  EndShaderMode();
  EndTextureMode();

  e->readIdx = writeIdx;
}

void SynapseTreeEffectResize(SynapseTreeEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
}

void SynapseTreeEffectUninit(SynapseTreeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

SynapseTreeConfig SynapseTreeConfigDefault(void) { return SynapseTreeConfig{}; }

void SynapseTreeRegisterParams(SynapseTreeConfig *cfg) {
  ModEngineRegisterParam("synapseTree.fov", &cfg->fov, 1.0f, 3.0f);
  ModEngineRegisterParam("synapseTree.foldOffset", &cfg->foldOffset, 0.2f,
                         1.0f);
  ModEngineRegisterParam("synapseTree.yFold", &cfg->yFold, 1.0f, 3.0f);
  ModEngineRegisterParam("synapseTree.branchThickness", &cfg->branchThickness,
                         0.01f, 0.2f);
  ModEngineRegisterParam("synapseTree.animSpeed", &cfg->animSpeed, 0.0f, 2.0f);
  ModEngineRegisterParam("synapseTree.driftSpeed", &cfg->driftSpeed, 0.0f,
                         1.0f);
  ModEngineRegisterParam("synapseTree.synapseIntensity", &cfg->synapseIntensity,
                         0.0f, 2.0f);
  ModEngineRegisterParam("synapseTree.synapseBounceFreq",
                         &cfg->synapseBounceFreq, 1.0f, 20.0f);
  ModEngineRegisterParam("synapseTree.synapsePulseFreq", &cfg->synapsePulseFreq,
                         1.0f, 15.0f);
  ModEngineRegisterParam("synapseTree.decayHalfLife", &cfg->decayHalfLife, 0.1f,
                         10.0f);
  ModEngineRegisterParam("synapseTree.trailBlur", &cfg->trailBlur, 0.0f, 1.0f);
  ModEngineRegisterParam("synapseTree.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("synapseTree.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("synapseTree.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("synapseTree.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("synapseTree.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("synapseTree.colorSpeed", &cfg->colorSpeed, 0.0f,
                         2.0f);
  ModEngineRegisterParam("synapseTree.colorStretch", &cfg->colorStretch, 0.1f,
                         5.0f);
  ModEngineRegisterParam("synapseTree.brightness", &cfg->brightness, 0.1f,
                         5.0f);
  ModEngineRegisterParam("synapseTree.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupSynapseTree(PostEffect *pe) {
  SynapseTreeEffectSetup(&pe->synapseTree, &pe->effects.synapseTree,
                         pe->currentDeltaTime, pe->fftTexture);
}

void SetupSynapseTreeBlend(PostEffect *pe) {
  BlendCompositorApply(
      pe->blendCompositor,
      pe->synapseTree.pingPong[pe->synapseTree.readIdx].texture,
      pe->effects.synapseTree.blendIntensity,
      pe->effects.synapseTree.blendMode);
}

void RenderSynapseTree(PostEffect *pe) {
  SynapseTreeEffectRender(&pe->synapseTree, &pe->effects.synapseTree,
                          pe->currentDeltaTime, pe->screenWidth,
                          pe->screenHeight);
}

// === UI ===

static void DrawSynapseTreeParams(EffectConfig *e, const ModSources *modSources,
                                  ImU32 categoryGlow) {
  (void)categoryGlow;
  SynapseTreeConfig *s = &e->synapseTree;

  // Raymarching
  ImGui::SeparatorText("Raymarching");
  ImGui::SliderInt("March Steps##synapseTree", &s->marchSteps, 40, 200);
  ImGui::SliderInt("Fold Depth##synapseTree", &s->foldIterations, 4, 12);
  ModulatableSlider("FOV##synapseTree", &s->fov, "synapseTree.fov", "%.1f",
                    modSources);

  // Shape
  ImGui::SeparatorText("Shape");
  ModulatableSlider("Branch Spread##synapseTree", &s->foldOffset,
                    "synapseTree.foldOffset", "%.2f", modSources);
  ModulatableSlider("Y Fold##synapseTree", &s->yFold, "synapseTree.yFold",
                    "%.2f", modSources);
  ModulatableSliderLog("Thickness##synapseTree", &s->branchThickness,
                       "synapseTree.branchThickness", "%.3f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Anim Speed##synapseTree", &s->animSpeed,
                    "synapseTree.animSpeed", "%.2f", modSources);
  ModulatableSlider("Drift Speed##synapseTree", &s->driftSpeed,
                    "synapseTree.driftSpeed", "%.2f", modSources);

  // Synapse
  ImGui::SeparatorText("Synapse");
  ModulatableSlider("Intensity##synapseTree", &s->synapseIntensity,
                    "synapseTree.synapseIntensity", "%.2f", modSources);
  ModulatableSlider("Bounce Freq##synapseTree", &s->synapseBounceFreq,
                    "synapseTree.synapseBounceFreq", "%.1f", modSources);
  ModulatableSlider("Pulse Freq##synapseTree", &s->synapsePulseFreq,
                    "synapseTree.synapsePulseFreq", "%.1f", modSources);

  // Trails
  ImGui::SeparatorText("Trails");
  ModulatableSlider("Decay Half-Life##synapseTree", &s->decayHalfLife,
                    "synapseTree.decayHalfLife", "%.1f", modSources);
  ModulatableSlider("Trail Blur##synapseTree", &s->trailBlur,
                    "synapseTree.trailBlur", "%.2f", modSources);

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##synapseTree", &s->baseFreq,
                    "synapseTree.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##synapseTree", &s->maxFreq,
                    "synapseTree.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##synapseTree", &s->gain, "synapseTree.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##synapseTree", &s->curve, "synapseTree.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##synapseTree", &s->baseBright,
                    "synapseTree.baseBright", "%.2f", modSources);

  // Color
  ImGui::SeparatorText("Color");
  ModulatableSlider("Color Speed##synapseTree", &s->colorSpeed,
                    "synapseTree.colorSpeed", "%.2f", modSources);
  ModulatableSlider("Color Stretch##synapseTree", &s->colorStretch,
                    "synapseTree.colorStretch", "%.2f", modSources);

  // Tonemap
  ImGui::SeparatorText("Tonemap");
  ModulatableSlider("Brightness##synapseTree", &s->brightness,
                    "synapseTree.brightness", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(synapseTree)
REGISTER_GENERATOR_FULL(TRANSFORM_SYNAPSE_TREE_BLEND, SynapseTree, synapseTree, "Synapse Tree",
                        SetupSynapseTreeBlend, SetupSynapseTree, RenderSynapseTree, 11,
                        DrawSynapseTreeParams, DrawOutput_synapseTree)
// clang-format on
