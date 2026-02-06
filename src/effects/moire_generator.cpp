// Moire generator effect module implementation
// Overlays rotatable line gratings to produce interference moire patterns

#include "moire_generator.h"
#include "automation/modulation_engine.h"
#include "render/color_lut.h"
#include "ui/ui_units.h"
#include <stddef.h>

// Map layer index to config field pointer
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

bool MoireGeneratorEffectInit(MoireGeneratorEffect *e) {
  e->shader = LoadShader(NULL, "shaders/moire_generator.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
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
  ColorConfig defaultGradient = {.mode = COLOR_MODE_GRADIENT};
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

void MoireGeneratorEffectSetup(MoireGeneratorEffect *e,
                               const MoireGeneratorConfig *cfg,
                               float deltaTime) {
  // Accumulate per-layer rotation and global time
  for (int i = 0; i < cfg->layerCount; i++) {
    const MoireLayerConfig *layer = GetLayer(cfg, i);
    e->layerAngles[i] += layer->rotationSpeed * deltaTime;
  }
  e->time += deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

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

  // Bind per-layer uniforms (all 4 slots, inactive layers receive defaults)
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

  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void MoireGeneratorEffectUninit(MoireGeneratorEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

MoireGeneratorConfig MoireGeneratorConfigDefault(void) {
  MoireGeneratorConfig cfg = {};

  cfg.layer0.frequency = 20.0f;
  cfg.layer0.angle = 0.0f;

  cfg.layer1.frequency = 22.0f;
  cfg.layer1.angle = 0.0873f;

  cfg.layer2.frequency = 24.0f;
  cfg.layer2.angle = 0.1745f;

  cfg.layer3.frequency = 26.0f;
  cfg.layer3.angle = 0.2618f;

  return cfg;
}

// NOLINTBEGIN(readability-function-size) — repetitive per-layer registration
void MoireGeneratorRegisterParams(MoireGeneratorConfig *cfg) {
  ModEngineRegisterParam("moireGenerator.colorIntensity", &cfg->colorIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("moireGenerator.globalBrightness",
                         &cfg->globalBrightness, 0.0f, 2.0f);
  ModEngineRegisterParam("moireGenerator.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);

  ModEngineRegisterParam("moireGenerator.layer0.frequency",
                         &cfg->layer0.frequency, 1.0f, 100.0f);
  ModEngineRegisterParam("moireGenerator.layer0.angle", &cfg->layer0.angle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("moireGenerator.layer0.rotationSpeed",
                         &cfg->layer0.rotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("moireGenerator.layer0.warpAmount",
                         &cfg->layer0.warpAmount, 0.0f, 0.5f);
  ModEngineRegisterParam("moireGenerator.layer0.scale", &cfg->layer0.scale,
                         0.5f, 4.0f);
  ModEngineRegisterParam("moireGenerator.layer0.phase", &cfg->layer0.phase,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);

  ModEngineRegisterParam("moireGenerator.layer1.frequency",
                         &cfg->layer1.frequency, 1.0f, 100.0f);
  ModEngineRegisterParam("moireGenerator.layer1.angle", &cfg->layer1.angle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("moireGenerator.layer1.rotationSpeed",
                         &cfg->layer1.rotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("moireGenerator.layer1.warpAmount",
                         &cfg->layer1.warpAmount, 0.0f, 0.5f);
  ModEngineRegisterParam("moireGenerator.layer1.scale", &cfg->layer1.scale,
                         0.5f, 4.0f);
  ModEngineRegisterParam("moireGenerator.layer1.phase", &cfg->layer1.phase,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);

  ModEngineRegisterParam("moireGenerator.layer2.frequency",
                         &cfg->layer2.frequency, 1.0f, 100.0f);
  ModEngineRegisterParam("moireGenerator.layer2.angle", &cfg->layer2.angle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("moireGenerator.layer2.rotationSpeed",
                         &cfg->layer2.rotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("moireGenerator.layer2.warpAmount",
                         &cfg->layer2.warpAmount, 0.0f, 0.5f);
  ModEngineRegisterParam("moireGenerator.layer2.scale", &cfg->layer2.scale,
                         0.5f, 4.0f);
  ModEngineRegisterParam("moireGenerator.layer2.phase", &cfg->layer2.phase,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);

  ModEngineRegisterParam("moireGenerator.layer3.frequency",
                         &cfg->layer3.frequency, 1.0f, 100.0f);
  ModEngineRegisterParam("moireGenerator.layer3.angle", &cfg->layer3.angle,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("moireGenerator.layer3.rotationSpeed",
                         &cfg->layer3.rotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);
  ModEngineRegisterParam("moireGenerator.layer3.warpAmount",
                         &cfg->layer3.warpAmount, 0.0f, 0.5f);
  ModEngineRegisterParam("moireGenerator.layer3.scale", &cfg->layer3.scale,
                         0.5f, 4.0f);
  ModEngineRegisterParam("moireGenerator.layer3.phase", &cfg->layer3.phase,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
}
// NOLINTEND(readability-function-size)
