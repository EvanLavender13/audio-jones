// Moire generator effect module implementation
// Overlays rotatable line gratings to produce interference moire patterns

#include "moire_generator.h"
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
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

// Map layer index to config field pointer (const)
static const MoireLayerConfig *GetLayer(const MoireGeneratorConfig *cfg,
                                        int i) {
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
static MoireLayerConfig *GetMutableLayer(MoireGeneratorConfig *cfg, int i) {
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

bool MoireGeneratorEffectInit(MoireGeneratorEffect *e,
                              const MoireGeneratorConfig *) {
  e->shader = LoadShader(NULL, "shaders/moire_generator.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->patternModeLoc = GetShaderLocation(e->shader, "patternMode");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->sharpModeLoc = GetShaderLocation(e->shader, "sharpMode");
  e->colorIntensityLoc = GetShaderLocation(e->shader, "colorIntensity");
  e->globalBrightnessLoc = GetShaderLocation(e->shader, "globalBrightness");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  for (int i = 0; i < 4; i++) {
    e->frequencyLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.frequency", i));
    e->angleLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.angle", i));
    e->warpAmountLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.warpAmount", i));
    e->scaleLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.scale", i));
    e->phaseLoc[i] =
        GetShaderLocation(e->shader, TextFormat("layer%d.phase", i));
  }

  // Use default gradient config since Init lacks a config parameter
  const ColorConfig defaultGradient = {.mode = COLOR_MODE_GRADIENT};
  e->gradientLUT = ColorLUTInit(&defaultGradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  for (int i = 0; i < 4; i++) {
    e->layerAngles[i] = 0.0f;
  }
  e->time = 0.0f;

  return true;
}

static void BindLayerUniforms(MoireGeneratorEffect *e,
                              const MoireGeneratorConfig *cfg) {
  for (int i = 0; i < 4; i++) {
    const MoireLayerConfig *layer = GetLayer(cfg, i);
    SetShaderValue(e->shader, e->frequencyLoc[i], &layer->frequency,
                   SHADER_UNIFORM_FLOAT);
    float totalAngle = layer->angle + e->layerAngles[i];
    SetShaderValue(e->shader, e->angleLoc[i], &totalAngle,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->warpAmountLoc[i], &layer->warpAmount,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->scaleLoc[i], &layer->scale,
                   SHADER_UNIFORM_FLOAT);
    SetShaderValue(e->shader, e->phaseLoc[i], &layer->phase,
                   SHADER_UNIFORM_FLOAT);
  }
}

void MoireGeneratorEffectSetup(MoireGeneratorEffect *e,
                               const MoireGeneratorConfig *cfg,
                               float deltaTime) {
  for (int i = 0; i < cfg->layerCount; i++) {
    const MoireLayerConfig *layer = GetLayer(cfg, i);
    e->layerAngles[i] += layer->rotationSpeed * deltaTime;
  }
  e->time += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  SetShaderValue(e->shader, e->patternModeLoc, &cfg->patternMode,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->layerCountLoc, &cfg->layerCount,
                 SHADER_UNIFORM_INT);

  int sharpInt = cfg->sharpMode ? 1 : 0;
  SetShaderValue(e->shader, e->sharpModeLoc, &sharpInt, SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->colorIntensityLoc, &cfg->colorIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->globalBrightnessLoc, &cfg->globalBrightness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);

  BindLayerUniforms(e, cfg);

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void MoireGeneratorEffectUninit(MoireGeneratorEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

MoireGeneratorConfig MoireGeneratorConfigDefault(void) {
  return MoireGeneratorConfig{};
}

void MoireGeneratorRegisterParams(MoireGeneratorConfig *cfg) {
  ModEngineRegisterParam("moireGenerator.colorIntensity", &cfg->colorIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("moireGenerator.globalBrightness",
                         &cfg->globalBrightness, 0.0f, 2.0f);
  ModEngineRegisterParam("moireGenerator.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);

  for (int i = 0; i < 4; i++) {
    MoireLayerConfig *l = GetMutableLayer(cfg, i);
    ModEngineRegisterParam(TextFormat("moireGenerator.layer%d.frequency", i),
                           &l->frequency, 1.0f, 100.0f);
    ModEngineRegisterParam(TextFormat("moireGenerator.layer%d.angle", i),
                           &l->angle, -ROTATION_OFFSET_MAX,
                           ROTATION_OFFSET_MAX);
    ModEngineRegisterParam(
        TextFormat("moireGenerator.layer%d.rotationSpeed", i),
        &l->rotationSpeed, -ROTATION_SPEED_MAX, ROTATION_SPEED_MAX);
    ModEngineRegisterParam(TextFormat("moireGenerator.layer%d.warpAmount", i),
                           &l->warpAmount, 0.0f, 0.5f);
    ModEngineRegisterParam(TextFormat("moireGenerator.layer%d.scale", i),
                           &l->scale, 0.5f, 4.0f);
    ModEngineRegisterParam(TextFormat("moireGenerator.layer%d.phase", i),
                           &l->phase, -ROTATION_OFFSET_MAX,
                           ROTATION_OFFSET_MAX);
  }
}

void SetupMoireGenerator(PostEffect *pe) {
  MoireGeneratorEffectSetup(&pe->moireGenerator, &pe->effects.moireGenerator,
                            pe->currentDeltaTime);
}

void SetupMoireGeneratorBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.moireGenerator.blendIntensity,
                       pe->effects.moireGenerator.blendMode);
}

// === UI ===

static void DrawMoireLayerControls(MoireLayerConfig *lyr, int n,
                                   const ModSources *modSources) {
  char label[64];
  char paramId[64];

  (void)snprintf(label, sizeof(label), "Layer %d", n);
  ImGui::SeparatorText(label);

  (void)snprintf(label, sizeof(label), "Frequency##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.frequency",
                 n);
  ModulatableSlider(label, &lyr->frequency, paramId, "%.1f", modSources);

  (void)snprintf(label, sizeof(label), "Angle##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.angle", n);
  ModulatableSliderAngleDeg(label, &lyr->angle, paramId, modSources);

  (void)snprintf(label, sizeof(label), "Rotation Speed##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId),
                 "moireGenerator.layer%d.rotationSpeed", n);
  ModulatableSliderSpeedDeg(label, &lyr->rotationSpeed, paramId, modSources);

  (void)snprintf(label, sizeof(label), "Warp##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.warpAmount",
                 n);
  ModulatableSlider(label, &lyr->warpAmount, paramId, "%.3f", modSources);

  (void)snprintf(label, sizeof(label), "Scale##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.scale", n);
  ModulatableSlider(label, &lyr->scale, paramId, "%.2f", modSources);

  (void)snprintf(label, sizeof(label), "Phase##moiregen_l%d", n);
  (void)snprintf(paramId, sizeof(paramId), "moireGenerator.layer%d.phase", n);
  ModulatableSliderAngleDeg(label, &lyr->phase, paramId, modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

static void DrawMoireGeneratorParams(EffectConfig *e,
                                     const ModSources *modSources,
                                     ImU32 categoryGlow) {
  MoireGeneratorConfig *mg = &e->moireGenerator;

  ImGui::Combo("Pattern##moiregen", &mg->patternMode,
               "Stripes\0Circles\0Grid\0");
  ImGui::SliderInt("Layers##moiregen", &mg->layerCount, 2, 4);
  ImGui::Checkbox("Sharp##moiregen", &mg->sharpMode);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  MoireLayerConfig *layers[] = {&mg->layer0, &mg->layer1, &mg->layer2,
                                &mg->layer3};
  for (int n = 0; n < mg->layerCount; n++) {
    DrawMoireLayerControls(layers[n], n, modSources);
  }

  // Color + Brightness (special: not in standard output)
  ImGui::SeparatorText("Color");
  ImGuiDrawColorMode(&mg->gradient);
  ModulatableSlider("Color Mix##moiregen", &mg->colorIntensity,
                    "moireGenerator.colorIntensity", "%.2f", modSources);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ModulatableSlider("Brightness##moiregen", &mg->globalBrightness,
                    "moireGenerator.globalBrightness", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(moireGenerator)
REGISTER_GENERATOR(TRANSFORM_MOIRE_GENERATOR_BLEND, MoireGenerator,
                   moireGenerator, "Moire Generator",
                   SetupMoireGeneratorBlend, SetupMoireGenerator, 12,
                   DrawMoireGeneratorParams, DrawOutput_moireGenerator)
// clang-format on
