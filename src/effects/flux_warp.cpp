#include "flux_warp.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

bool FluxWarpEffectInit(FluxWarpEffect *e) {
  e->shader = LoadShader(NULL, "shaders/flux_warp.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->warpStrengthLoc = GetShaderLocation(e->shader, "warpStrength");
  e->cellScaleLoc = GetShaderLocation(e->shader, "cellScale");
  e->couplingLoc = GetShaderLocation(e->shader, "coupling");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->divisorSpeedLoc = GetShaderLocation(e->shader, "divisorSpeed");
  e->gateSpeedLoc = GetShaderLocation(e->shader, "gateSpeed");

  e->time = 0.0f;

  return true;
}

void FluxWarpEffectSetup(FluxWarpEffect *e, const FluxWarpConfig *cfg,
                         float deltaTime, int screenWidth, int screenHeight) {
  e->time += cfg->animSpeed * deltaTime;

  const float resolution[2] = {(float)screenWidth, (float)screenHeight};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->warpStrengthLoc, &cfg->warpStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellScaleLoc, &cfg->cellScale,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->couplingLoc, &cfg->coupling,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->divisorSpeedLoc, &cfg->divisorSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gateSpeedLoc, &cfg->gateSpeed,
                 SHADER_UNIFORM_FLOAT);
}

void FluxWarpEffectUninit(const FluxWarpEffect *e) { UnloadShader(e->shader); }

void FluxWarpRegisterParams(FluxWarpConfig *cfg) {
  ModEngineRegisterParam("fluxWarp.warpStrength", &cfg->warpStrength, 0.0f,
                         0.5f);
  ModEngineRegisterParam("fluxWarp.cellScale", &cfg->cellScale, 1.0f, 20.0f);
  ModEngineRegisterParam("fluxWarp.coupling", &cfg->coupling, 0.0f, 1.0f);
  ModEngineRegisterParam("fluxWarp.waveFreq", &cfg->waveFreq, 10.0f, 500.0f);
  ModEngineRegisterParam("fluxWarp.animSpeed", &cfg->animSpeed, 0.0f, 2.0f);
}

// === UI ===

static void DrawFluxWarpParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  ModulatableSlider("Strength##fluxwarp", &e->fluxWarp.warpStrength,
                    "fluxWarp.warpStrength", "%.3f", ms);
  ModulatableSlider("Cell Scale##fluxwarp", &e->fluxWarp.cellScale,
                    "fluxWarp.cellScale", "%.1f", ms);
  ModulatableSlider("Coupling##fluxwarp", &e->fluxWarp.coupling,
                    "fluxWarp.coupling", "%.2f", ms);
  ModulatableSlider("Wave Freq##fluxwarp", &e->fluxWarp.waveFreq,
                    "fluxWarp.waveFreq", "%.1f", ms);
  ModulatableSlider("Anim Speed##fluxwarp", &e->fluxWarp.animSpeed,
                    "fluxWarp.animSpeed", "%.2f", ms);
  ImGui::SliderFloat("Divisor Speed##fluxwarp", &e->fluxWarp.divisorSpeed, 0.0f,
                     1.0f, "%.2f");
  ImGui::SliderFloat("Gate Speed##fluxwarp", &e->fluxWarp.gateSpeed, 0.0f, 0.5f,
                     "%.2f");
}

void SetupFluxWarp(PostEffect *pe) {
  FluxWarpEffectSetup(&pe->fluxWarp, &pe->effects.fluxWarp,
                      pe->currentDeltaTime, pe->screenWidth, pe->screenHeight);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_FLUX_WARP, FluxWarp, fluxWarp, "Flux Warp",
                "WARP", 1, EFFECT_FLAG_NONE, SetupFluxWarp, NULL, DrawFluxWarpParams)
// clang-format on
