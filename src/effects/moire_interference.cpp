#include "moire_interference.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

// Map layer index to const config field pointer
static const MoireInterferenceLayerConfig *
GetLayer(const MoireInterferenceConfig *cfg, int i) {
  switch (i) {
  case 0:
    return &cfg->layer0;
  case 1:
    return &cfg->layer1;
  case 2:
    return &cfg->layer2;
  case 3:
    return &cfg->layer3;
  default:
    return &cfg->layer0;
  }
}

// Map layer index to mutable config field pointer
static MoireInterferenceLayerConfig *
GetMutableLayer(MoireInterferenceConfig *cfg, int i) {
  switch (i) {
  case 0:
    return &cfg->layer0;
  case 1:
    return &cfg->layer1;
  case 2:
    return &cfg->layer2;
  case 3:
    return &cfg->layer3;
  default:
    return &cfg->layer0;
  }
}

bool MoireInterferenceEffectInit(MoireInterferenceEffect *e) {
  e->shader = LoadShader(NULL, "shaders/moire_interference.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->patternModeLoc = GetShaderLocation(e->shader, "patternMode");
  e->profileModeLoc = GetShaderLocation(e->shader, "profileMode");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->centerXLoc = GetShaderLocation(e->shader, "centerX");
  e->centerYLoc = GetShaderLocation(e->shader, "centerY");
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");

  for (int i = 0; i < 4; i++) {
    e->frequencyLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.frequency", i));
    e->angleLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.angle", i));
    e->amplitudeLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.amplitude", i));
    e->phaseLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.phase", i));
  }

  for (int i = 0; i < 4; i++) {
    e->layerAngles[i] = 0.0f;
  }

  return true;
}

void MoireInterferenceEffectSetup(MoireInterferenceEffect *e,
                                  const MoireInterferenceConfig *cfg,
                                  float deltaTime) {
  // Accumulate per-layer rotation
  for (int i = 0; i < cfg->layerCount; i++) {
    e->layerAngles[i] += GetLayer(cfg, i)->rotationSpeed * deltaTime;
  }

  // Bind global uniforms
  SetShaderValue(e->shader, e->patternModeLoc, &cfg->patternMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->profileModeLoc, &cfg->profileMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->layerCountLoc, &cfg->layerCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->centerXLoc, &cfg->centerX, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->centerYLoc, &cfg->centerY, SHADER_UNIFORM_FLOAT);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  // Bind per-layer uniforms
  for (int i = 0; i < 4; i++) {
    const MoireInterferenceLayerConfig *layer = GetLayer(cfg, i);
    SetShaderValue(e->shader, e->frequencyLoc[i], &layer->frequency,
                   SHADER_UNIFORM_FLOAT);
    float totalAngle = layer->angle + e->layerAngles[i];
    SetShaderValue(e->shader, e->angleLoc[i], &totalAngle,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->amplitudeLoc[i], &layer->amplitude,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->phaseLoc[i], &layer->phase,
                   SHADER_UNIFORM_FLOAT);
  }
}

void MoireInterferenceEffectUninit(const MoireInterferenceEffect *e) {
  UnloadShader(e->shader);
}

void MoireInterferenceRegisterParams(MoireInterferenceConfig *cfg) {
  for (int i = 0; i < 4; i++) {
    MoireInterferenceLayerConfig *l = GetMutableLayer(cfg, i);
    ModEngineRegisterParam(TextFormat("moireInterference.layer%d.frequency", i),
                           &l->frequency, 1.0f, 30.0f);
    ModEngineRegisterParam(TextFormat("moireInterference.layer%d.angle", i),
                           &l->angle, -ROTATION_OFFSET_MAX,
                           ROTATION_OFFSET_MAX);
    ModEngineRegisterParam(
        TextFormat("moireInterference.layer%d.rotationSpeed", i),
        &l->rotationSpeed, -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
    ModEngineRegisterParam(TextFormat("moireInterference.layer%d.amplitude", i),
                           &l->amplitude, 0.0f, 0.15f);
    ModEngineRegisterParam(TextFormat("moireInterference.layer%d.phase", i),
                           &l->phase, -ROTATION_OFFSET_MAX,
                           ROTATION_OFFSET_MAX);
  }
}

// === UI ===

static void
DrawMoireInterferenceLayerControls(MoireInterferenceLayerConfig *lyr, int n,
                                   const ModSources *modSources) {
  char label[64];
  char paramId[64];

  (void)snprintf(label, sizeof(label), "Layer %d", n);
  ImGui::SeparatorText(label);

  (void)snprintf(label, sizeof(label), "Frequency##moireint_l%d", n);
  (void)snprintf(paramId, sizeof(paramId),
                 "moireInterference.layer%d.frequency", n);
  ModulatableSlider(label, &lyr->frequency, paramId, "%.1f", modSources);

  (void)snprintf(label, sizeof(label), "Angle##moireint_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireInterference.layer%d.angle",
                 n);
  ModulatableSliderAngleDeg(label, &lyr->angle, paramId, modSources);

  (void)snprintf(label, sizeof(label), "Rotation Speed##moireint_l%d", n);
  (void)snprintf(paramId, sizeof(paramId),
                 "moireInterference.layer%d.rotationSpeed", n);
  ModulatableSliderSpeedDeg(label, &lyr->rotationSpeed, paramId, modSources);

  (void)snprintf(label, sizeof(label), "Amplitude##moireint_l%d", n);
  (void)snprintf(paramId, sizeof(paramId),
                 "moireInterference.layer%d.amplitude", n);
  ModulatableSlider(label, &lyr->amplitude, paramId, "%.3f", modSources);

  (void)snprintf(label, sizeof(label), "Phase##moireint_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireInterference.layer%d.phase",
                 n);
  ModulatableSliderAngleDeg(label, &lyr->phase, paramId, modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

static void DrawMoireInterferenceParams(EffectConfig *e, const ModSources *ms,
                                        ImU32 glow) {
  MoireInterferenceConfig *mi = &e->moireInterference;

  ImGui::Combo("Pattern##moireint", &mi->patternMode,
               "Stripes\0Circles\0Grid\0");
  ImGui::Combo("Profile##moireint", &mi->profileMode,
               "Sine\0Square\0Triangle\0Sawtooth\0");
  ImGui::SliderInt("Layers##moireint", &mi->layerCount, 2, 4);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  MoireInterferenceLayerConfig *layers[] = {&mi->layer0, &mi->layer1,
                                            &mi->layer2, &mi->layer3};
  for (int n = 0; n < mi->layerCount; n++) {
    DrawMoireInterferenceLayerControls(layers[n], n, ms);
  }

  if (TreeNodeAccented("Center##moireint", glow)) {
    ImGui::SliderFloat("X##moireintcenter", &mi->centerX, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Y##moireintcenter", &mi->centerY, 0.0f, 1.0f, "%.2f");
    TreeNodeAccentedPop();
  }
}

void SetupMoireInterference(PostEffect *pe) {
  MoireInterferenceEffectSetup(&pe->moireInterference,
                               &pe->effects.moireInterference,
                               pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(
    TRANSFORM_MOIRE_INTERFERENCE, MoireInterference, moireInterference,
    "Moire Interference", "WARP", 1, EFFECT_FLAG_NONE,
    SetupMoireInterference, NULL, DrawMoireInterferenceParams)
// clang-format on
