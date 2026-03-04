// Byzantine effect module implementation
// Dual-pass cellular automaton with zoom-reseed cycles and FFT-driven display

#include "byzantine.h"
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
#include "rlgl.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <math.h>
#include <stddef.h>

static void InitPingPong(ByzantineEffect *e, int width, int height) {
  RenderUtilsInitTextureHDR(&e->pingPong[0], width, height, "BYZANTINE");
  RenderUtilsInitTextureHDR(&e->pingPong[1], width, height, "BYZANTINE");
}

static void UnloadPingPong(ByzantineEffect *e) {
  UnloadRenderTexture(e->pingPong[0]);
  UnloadRenderTexture(e->pingPong[1]);
}

static void CacheLocations(ByzantineEffect *e) {
  // Sim shader locations
  e->simResolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->simFrameCountLoc = GetShaderLocation(e->shader, "frameCount");
  e->simCycleLengthLoc = GetShaderLocation(e->shader, "cycleLength");
  e->simZoomAmountLoc = GetShaderLocation(e->shader, "zoomAmount");
  e->simCenterLoc = GetShaderLocation(e->shader, "center");
  e->simRotationLoc = GetShaderLocation(e->shader, "rotation");
  e->simTwistLoc = GetShaderLocation(e->shader, "twist");

  // Display shader locations
  e->dispResolutionLoc = GetShaderLocation(e->displayShader, "resolution");
  e->dispCycleProgressLoc =
      GetShaderLocation(e->displayShader, "cycleProgress");
  e->dispZoomAmountLoc = GetShaderLocation(e->displayShader, "zoomAmount");
  e->dispCenterLoc = GetShaderLocation(e->displayShader, "center");
  e->dispCycleRotationLoc =
      GetShaderLocation(e->displayShader, "cycleRotation");
  e->dispCycleTwistLoc = GetShaderLocation(e->displayShader, "cycleTwist");
  e->dispGradientLUTLoc = GetShaderLocation(e->displayShader, "gradientLUT");
}

bool ByzantineEffectInit(ByzantineEffect *e, const ByzantineConfig *cfg,
                         int width, int height) {
  e->shader = LoadShader(NULL, "shaders/byzantine_sim.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->displayShader = LoadShader(NULL, "shaders/byzantine_display.fs");
  if (e->displayShader.id == 0) {
    UnloadShader(e->shader);
    return false;
  }

  CacheLocations(e);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    UnloadShader(e->displayShader);
    return false;
  }

  InitPingPong(e, width, height);
  RenderUtilsClearTexture(&e->pingPong[0]);
  RenderUtilsClearTexture(&e->pingPong[1]);
  e->readIdx = 0;
  e->frameCount = 0;
  e->cachedCycleLen = (int)cfg->cycleLength;

  return true;
}

void ByzantineEffectSetup(ByzantineEffect *e, const ByzantineConfig *cfg,
                          float deltaTime) {
  e->cachedCycleLen = (int)cfg->cycleLength;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->simResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);
  SetShaderValue(e->displayShader, e->dispResolutionLoc, resolution,
                 SHADER_UNIFORM_VEC2);

  int cycleLen = e->cachedCycleLen;
  SetShaderValue(e->shader, e->simCycleLengthLoc, &cycleLen,
                 SHADER_UNIFORM_INT);

  // Sim uniforms
  SetShaderValue(e->shader, e->simZoomAmountLoc, &cfg->zoomAmount,
                 SHADER_UNIFORM_FLOAT);
  float center[2] = {cfg->centerX, cfg->centerY};
  SetShaderValue(e->shader, e->simCenterLoc, center, SHADER_UNIFORM_VEC2);
  float perStepRotation = cfg->rotationSpeed * deltaTime;
  float perStepTwist = cfg->twistSpeed * deltaTime;
  SetShaderValue(e->shader, e->simRotationLoc, &perStepRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->simTwistLoc, &perStepTwist,
                 SHADER_UNIFORM_FLOAT);

  float cycleRotation = perStepRotation * cfg->cycleLength;
  float cycleTwist = perStepTwist * cfg->cycleLength;
  SetShaderValue(e->displayShader, e->dispCycleRotationLoc, &cycleRotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispCycleTwistLoc, &cycleTwist,
                 SHADER_UNIFORM_FLOAT);

  // Display uniforms
  SetShaderValue(e->displayShader, e->dispZoomAmountLoc, &cfg->zoomAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->displayShader, e->dispCenterLoc, center,
                 SHADER_UNIFORM_VEC2);
}

void ByzantineEffectRender(ByzantineEffect *e, PostEffect *pe) {
  // Sim pass: one step per display frame (matches Shadertoy behavior)
  SetShaderValue(e->shader, e->simFrameCountLoc, &e->frameCount,
                 SHADER_UNIFORM_INT);

  int writeIdx = 1 - e->readIdx;
  BeginTextureMode(e->pingPong[writeIdx]);
  rlDisableColorBlend(); // Sim must overwrite, not alpha-blend
  BeginShaderMode(e->shader);
  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture,
                                pe->screenWidth, pe->screenHeight);
  EndShaderMode();
  rlEnableColorBlend();
  EndTextureMode();
  e->readIdx = writeIdx;
  e->frameCount++;

  // Display pass
  float cycleProgress = (float)((e->frameCount - 1) % e->cachedCycleLen) /
                        (float)e->cachedCycleLen;
  SetShaderValue(e->displayShader, e->dispCycleProgressLoc, &cycleProgress,
                 SHADER_UNIFORM_FLOAT);

  BeginTextureMode(pe->generatorScratch);
  BeginShaderMode(e->displayShader);
  SetShaderValueTexture(e->displayShader, e->dispGradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  RenderUtilsDrawFullscreenQuad(e->pingPong[e->readIdx].texture,
                                pe->screenWidth, pe->screenHeight);
  EndShaderMode();
  EndTextureMode();
}

void ByzantineEffectResize(ByzantineEffect *e, int width, int height) {
  UnloadPingPong(e);
  InitPingPong(e, width, height);
  e->readIdx = 0;
  e->frameCount = 0;
}

void ByzantineEffectUninit(ByzantineEffect *e) {
  UnloadShader(e->shader);
  UnloadShader(e->displayShader);
  ColorLUTUninit(e->gradientLUT);
  UnloadPingPong(e);
}

ByzantineConfig ByzantineConfigDefault(void) { return ByzantineConfig{}; }

void ByzantineRegisterParams(ByzantineConfig *cfg) {
  ModEngineRegisterParam("byzantine.cycleLength", &cfg->cycleLength, 60.0f,
                         600.0f);
  ModEngineRegisterParam("byzantine.zoomAmount", &cfg->zoomAmount, 1.0f, 4.0f);
  ModEngineRegisterParam("byzantine.centerX", &cfg->centerX, 0.0f, 1.0f);
  ModEngineRegisterParam("byzantine.centerY", &cfg->centerY, 0.0f, 1.0f);
  ModEngineRegisterParam("byzantine.rotationSpeed", &cfg->rotationSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("byzantine.twistSpeed", &cfg->twistSpeed,
                         -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
  ModEngineRegisterParam("byzantine.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupByzantine(PostEffect *pe) {
  ByzantineEffectSetup(&pe->byzantine, &pe->effects.byzantine,
                       pe->currentDeltaTime);
}

void SetupByzantineBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.byzantine.blendIntensity,
                       pe->effects.byzantine.blendMode);
}

void RenderByzantine(PostEffect *pe) {
  ByzantineEffectRender(&pe->byzantine, pe);
}

// === UI ===

static void DrawByzantineParams(EffectConfig *e, const ModSources *modSources,
                                ImU32 categoryGlow) {
  (void)categoryGlow;
  ByzantineConfig *b = &e->byzantine;

  // Simulation
  ImGui::SeparatorText("Simulation");
  ModulatableSlider("Cycle Length##byzantine", &b->cycleLength,
                    "byzantine.cycleLength", "%.0f", modSources);
  ModulatableSlider("Zoom Amount##byzantine", &b->zoomAmount,
                    "byzantine.zoomAmount", "%.1f", modSources);
  ModulatableSlider("Center X##byzantine", &b->centerX, "byzantine.centerX",
                    "%.2f", modSources);
  ModulatableSlider("Center Y##byzantine", &b->centerY, "byzantine.centerY",
                    "%.2f", modSources);
  ModulatableSliderSpeedDeg("Rotation##byzantine", &b->rotationSpeed,
                            "byzantine.rotationSpeed", modSources);
  ModulatableSliderSpeedDeg("Twist##byzantine", &b->twistSpeed,
                            "byzantine.twistSpeed", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(byzantine)
REGISTER_GENERATOR_FULL(TRANSFORM_BYZANTINE_BLEND, Byzantine, byzantine,
                        "Byzantine", SetupByzantineBlend, SetupByzantine,
                        RenderByzantine, 12, DrawByzantineParams,
                        DrawOutput_byzantine)
// clang-format on
