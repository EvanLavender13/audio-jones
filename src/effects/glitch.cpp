// Glitch video corruption effect module implementation

#include "glitch.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/post_effect.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

static void CacheLocations(GlitchEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->frameLoc = GetShaderLocation(e->shader, "frame");
  e->analogIntensityLoc = GetShaderLocation(e->shader, "analogIntensity");
  e->aberrationLoc = GetShaderLocation(e->shader, "aberration");
  e->blockThresholdLoc = GetShaderLocation(e->shader, "blockThreshold");
  e->blockOffsetLoc = GetShaderLocation(e->shader, "blockOffset");
  e->vhsEnabledLoc = GetShaderLocation(e->shader, "vhsEnabled");
  e->trackingBarIntensityLoc =
      GetShaderLocation(e->shader, "trackingBarIntensity");
  e->scanlineNoiseIntensityLoc =
      GetShaderLocation(e->shader, "scanlineNoiseIntensity");
  e->colorDriftIntensityLoc =
      GetShaderLocation(e->shader, "colorDriftIntensity");
  e->scanlineAmountLoc = GetShaderLocation(e->shader, "scanlineAmount");
  e->noiseAmountLoc = GetShaderLocation(e->shader, "noiseAmount");
  e->datamoshEnabledLoc = GetShaderLocation(e->shader, "datamoshEnabled");
  e->datamoshIntensityLoc = GetShaderLocation(e->shader, "datamoshIntensity");
  e->datamoshMinLoc = GetShaderLocation(e->shader, "datamoshMin");
  e->datamoshMaxLoc = GetShaderLocation(e->shader, "datamoshMax");
  e->datamoshSpeedLoc = GetShaderLocation(e->shader, "datamoshSpeed");
  e->datamoshBandsLoc = GetShaderLocation(e->shader, "datamoshBands");
  e->rowSliceEnabledLoc = GetShaderLocation(e->shader, "rowSliceEnabled");
  e->rowSliceIntensityLoc = GetShaderLocation(e->shader, "rowSliceIntensity");
  e->rowSliceBurstFreqLoc = GetShaderLocation(e->shader, "rowSliceBurstFreq");
  e->rowSliceBurstPowerLoc = GetShaderLocation(e->shader, "rowSliceBurstPower");
  e->rowSliceColumnsLoc = GetShaderLocation(e->shader, "rowSliceColumns");
  e->colSliceEnabledLoc = GetShaderLocation(e->shader, "colSliceEnabled");
  e->colSliceIntensityLoc = GetShaderLocation(e->shader, "colSliceIntensity");
  e->colSliceBurstFreqLoc = GetShaderLocation(e->shader, "colSliceBurstFreq");
  e->colSliceBurstPowerLoc = GetShaderLocation(e->shader, "colSliceBurstPower");
  e->colSliceRowsLoc = GetShaderLocation(e->shader, "colSliceRows");
  e->diagonalBandsEnabledLoc =
      GetShaderLocation(e->shader, "diagonalBandsEnabled");
  e->diagonalBandCountLoc = GetShaderLocation(e->shader, "diagonalBandCount");
  e->diagonalBandDisplaceLoc =
      GetShaderLocation(e->shader, "diagonalBandDisplace");
  e->diagonalBandSpeedLoc = GetShaderLocation(e->shader, "diagonalBandSpeed");
  e->blockMaskEnabledLoc = GetShaderLocation(e->shader, "blockMaskEnabled");
  e->blockMaskIntensityLoc = GetShaderLocation(e->shader, "blockMaskIntensity");
  e->blockMaskMinSizeLoc = GetShaderLocation(e->shader, "blockMaskMinSize");
  e->blockMaskMaxSizeLoc = GetShaderLocation(e->shader, "blockMaskMaxSize");
  e->blockMaskTintLoc = GetShaderLocation(e->shader, "blockMaskTint");
  e->temporalJitterEnabledLoc =
      GetShaderLocation(e->shader, "temporalJitterEnabled");
  e->temporalJitterAmountLoc =
      GetShaderLocation(e->shader, "temporalJitterAmount");
  e->temporalJitterGateLoc = GetShaderLocation(e->shader, "temporalJitterGate");
  e->blockMultiplyEnabledLoc =
      GetShaderLocation(e->shader, "blockMultiplyEnabled");
  e->blockMultiplySizeLoc = GetShaderLocation(e->shader, "blockMultiplySize");
  e->blockMultiplyControlLoc =
      GetShaderLocation(e->shader, "blockMultiplyControl");
  e->blockMultiplyIterationsLoc =
      GetShaderLocation(e->shader, "blockMultiplyIterations");
  e->blockMultiplyIntensityLoc =
      GetShaderLocation(e->shader, "blockMultiplyIntensity");
}

bool GlitchEffectInit(GlitchEffect *e) {
  e->shader = LoadShader(NULL, "shaders/glitch.fs");
  if (e->shader.id == 0) {
    return false;
  }

  CacheLocations(e);
  e->time = 0.0f;
  e->frame = 0;

  return true;
}

static void SetupAnalog(const GlitchEffect *e, const GlitchConfig *cfg) {
  SetShaderValue(e->shader, e->analogIntensityLoc, &cfg->analogIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->aberrationLoc, &cfg->aberration,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blockThresholdLoc, &cfg->blockThreshold,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blockOffsetLoc, &cfg->blockOffset,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupVhs(const GlitchEffect *e, const GlitchConfig *cfg) {
  int vhsEnabled = cfg->vhsEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->vhsEnabledLoc, &vhsEnabled, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->trackingBarIntensityLoc,
                 &cfg->trackingBarIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scanlineNoiseIntensityLoc,
                 &cfg->scanlineNoiseIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorDriftIntensityLoc,
                 &cfg->colorDriftIntensity, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scanlineAmountLoc, &cfg->scanlineAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->noiseAmountLoc, &cfg->noiseAmount,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupDatamosh(const GlitchEffect *e, const GlitchConfig *cfg) {
  int datamoshEnabled = cfg->datamoshEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->datamoshEnabledLoc, &datamoshEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->datamoshIntensityLoc, &cfg->datamoshIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->datamoshMinLoc, &cfg->datamoshMin,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->datamoshMaxLoc, &cfg->datamoshMax,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->datamoshSpeedLoc, &cfg->datamoshSpeed,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->datamoshBandsLoc, &cfg->datamoshBands,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupSlice(const GlitchEffect *e, const GlitchConfig *cfg) {
  int rowSliceEnabled = cfg->rowSliceEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->rowSliceEnabledLoc, &rowSliceEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->rowSliceIntensityLoc, &cfg->rowSliceIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rowSliceBurstFreqLoc, &cfg->rowSliceBurstFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rowSliceBurstPowerLoc, &cfg->rowSliceBurstPower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rowSliceColumnsLoc, &cfg->rowSliceColumns,
                 SHADER_UNIFORM_FLOAT);

  int colSliceEnabled = cfg->colSliceEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->colSliceEnabledLoc, &colSliceEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->colSliceIntensityLoc, &cfg->colSliceIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colSliceBurstFreqLoc, &cfg->colSliceBurstFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colSliceBurstPowerLoc, &cfg->colSliceBurstPower,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colSliceRowsLoc, &cfg->colSliceRows,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupDiagonalBands(const GlitchEffect *e, const GlitchConfig *cfg) {
  int diagonalBandsEnabled = cfg->diagonalBandsEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->diagonalBandsEnabledLoc, &diagonalBandsEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->diagonalBandCountLoc, &cfg->diagonalBandCount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diagonalBandDisplaceLoc,
                 &cfg->diagonalBandDisplace, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diagonalBandSpeedLoc, &cfg->diagonalBandSpeed,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupBlockMask(const GlitchEffect *e, const GlitchConfig *cfg) {
  int blockMaskEnabled = cfg->blockMaskEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->blockMaskEnabledLoc, &blockMaskEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blockMaskIntensityLoc, &cfg->blockMaskIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blockMaskMinSizeLoc, &cfg->blockMaskMinSize,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blockMaskMaxSizeLoc, &cfg->blockMaskMaxSize,
                 SHADER_UNIFORM_INT);
  const float blockMaskTint[3] = {cfg->blockMaskTintR, cfg->blockMaskTintG,
                                  cfg->blockMaskTintB};
  SetShaderValue(e->shader, e->blockMaskTintLoc, blockMaskTint,
                 SHADER_UNIFORM_VEC3);
}

static void SetupTemporalJitter(const GlitchEffect *e,
                                const GlitchConfig *cfg) {
  int temporalJitterEnabled = cfg->temporalJitterEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->temporalJitterEnabledLoc, &temporalJitterEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->temporalJitterAmountLoc,
                 &cfg->temporalJitterAmount, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->temporalJitterGateLoc, &cfg->temporalJitterGate,
                 SHADER_UNIFORM_FLOAT);
}

static void SetupBlockMultiply(const GlitchEffect *e, const GlitchConfig *cfg) {
  int blockMultiplyEnabled = cfg->blockMultiplyEnabled ? 1 : 0;
  SetShaderValue(e->shader, e->blockMultiplyEnabledLoc, &blockMultiplyEnabled,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blockMultiplySizeLoc, &cfg->blockMultiplySize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blockMultiplyControlLoc,
                 &cfg->blockMultiplyControl, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->blockMultiplyIterationsLoc,
                 &cfg->blockMultiplyIterations, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->blockMultiplyIntensityLoc,
                 &cfg->blockMultiplyIntensity, SHADER_UNIFORM_FLOAT);
}

void GlitchEffectSetup(GlitchEffect *e, const GlitchConfig *cfg,
                       float deltaTime) {
  e->time += deltaTime;
  e->frame++;

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->frameLoc, &e->frame, SHADER_UNIFORM_INT);

  SetupAnalog(e, cfg);
  SetupVhs(e, cfg);
  SetupDatamosh(e, cfg);
  SetupSlice(e, cfg);
  SetupDiagonalBands(e, cfg);
  SetupBlockMask(e, cfg);
  SetupTemporalJitter(e, cfg);
  SetupBlockMultiply(e, cfg);
}

void GlitchEffectUninit(const GlitchEffect *e) { UnloadShader(e->shader); }

void GlitchRegisterParams(GlitchConfig *cfg) {
  ModEngineRegisterParam("glitch.analogIntensity", &cfg->analogIntensity, 0.0f,
                         1.0f);
  ModEngineRegisterParam("glitch.blockThreshold", &cfg->blockThreshold, 0.0f,
                         0.9f);
  ModEngineRegisterParam("glitch.aberration", &cfg->aberration, 0.0f, 20.0f);
  ModEngineRegisterParam("glitch.blockOffset", &cfg->blockOffset, 0.0f, 0.5f);
  ModEngineRegisterParam("glitch.datamoshIntensity", &cfg->datamoshIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("glitch.datamoshMin", &cfg->datamoshMin, 4.0f, 32.0f);
  ModEngineRegisterParam("glitch.datamoshMax", &cfg->datamoshMax, 16.0f,
                         128.0f);
  ModEngineRegisterParam("glitch.rowSliceIntensity", &cfg->rowSliceIntensity,
                         0.0f, 0.5f);
  ModEngineRegisterParam("glitch.colSliceIntensity", &cfg->colSliceIntensity,
                         0.0f, 0.5f);
  ModEngineRegisterParam("glitch.diagonalBandDisplace",
                         &cfg->diagonalBandDisplace, 0.0f, 0.1f);
  ModEngineRegisterParam("glitch.blockMaskIntensity", &cfg->blockMaskIntensity,
                         0.0f, 1.0f);
  ModEngineRegisterParam("glitch.temporalJitterAmount",
                         &cfg->temporalJitterAmount, 0.0f, 0.1f);
  ModEngineRegisterParam("glitch.temporalJitterGate", &cfg->temporalJitterGate,
                         0.0f, 1.0f);
  ModEngineRegisterParam("glitch.blockMultiplySize", &cfg->blockMultiplySize,
                         4.0f, 64.0f);
  ModEngineRegisterParam("glitch.blockMultiplyControl",
                         &cfg->blockMultiplyControl, 0.0f, 1.0f);
  ModEngineRegisterParam("glitch.blockMultiplyIntensity",
                         &cfg->blockMultiplyIntensity, 0.0f, 1.0f);
}

// === UI ===

static void DrawGlitchAnalog(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Analog");
  ModulatableSlider("Intensity##analog", &g->analogIntensity,
                    "glitch.analogIntensity", "%.3f", ms);
  ModulatableSlider("Aberration##analog", &g->aberration, "glitch.aberration",
                    "%.1f px", ms);
}

static void DrawGlitchDigital(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Digital");
  ModulatableSlider("Block Threshold##digital", &g->blockThreshold,
                    "glitch.blockThreshold", "%.2f", ms);
  ModulatableSlider("Block Offset##digital", &g->blockOffset,
                    "glitch.blockOffset", "%.2f", ms);
}

static void DrawGlitchVHS(GlitchConfig *g) {
  ImGui::SeparatorText("VHS");
  ImGui::Checkbox("Enabled##vhs", &g->vhsEnabled);
  if (g->vhsEnabled) {
    ImGui::SliderFloat("Tracking Bars##vhs", &g->trackingBarIntensity, 0.0f,
                       0.05f, "%.3f");
    ImGui::SliderFloat("Scanline Noise##vhs", &g->scanlineNoiseIntensity, 0.0f,
                       0.02f, "%.4f");
    ImGui::SliderFloat("Color Drift##vhs", &g->colorDriftIntensity, 0.0f, 2.0f,
                       "%.2f");
  }
}

static void DrawGlitchDatamosh(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Datamosh");
  ImGui::Checkbox("Enabled##datamosh", &g->datamoshEnabled);
  if (g->datamoshEnabled) {
    ModulatableSlider("Intensity##datamosh", &g->datamoshIntensity,
                      "glitch.datamoshIntensity", "%.2f", ms);
    ModulatableSlider("Min Res##datamosh", &g->datamoshMin,
                      "glitch.datamoshMin", "%.0f", ms);
    ModulatableSlider("Max Res##datamosh", &g->datamoshMax,
                      "glitch.datamoshMax", "%.0f", ms);
    ImGui::SliderFloat("Speed##datamosh", &g->datamoshSpeed, 1.0f, 30.0f,
                       "%.1f");
    ImGui::SliderFloat("Bands##datamosh", &g->datamoshBands, 1.0f, 32.0f,
                       "%.0f");
  }
}

static void DrawGlitchSlice(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Slice");
  ImGui::Text("Row (Horizontal)");
  ImGui::Checkbox("Enabled##rowslice", &g->rowSliceEnabled);
  if (g->rowSliceEnabled) {
    ModulatableSlider("Intensity##rowslice", &g->rowSliceIntensity,
                      "glitch.rowSliceIntensity", "%.3f", ms);
    ImGui::SliderFloat("Burst Freq##rowslice", &g->rowSliceBurstFreq, 0.5f,
                       20.0f, "%.1f Hz");
    ImGui::SliderFloat("Burst Power##rowslice", &g->rowSliceBurstPower, 1.0f,
                       15.0f, "%.1f");
    ImGui::SliderFloat("Column Width##rowslice", &g->rowSliceColumns, 8.0f,
                       128.0f, "%.0f");
  }
  ImGui::Spacing();
  ImGui::Text("Column (Vertical)");
  ImGui::Checkbox("Enabled##colslice", &g->colSliceEnabled);
  if (g->colSliceEnabled) {
    ModulatableSlider("Intensity##colslice", &g->colSliceIntensity,
                      "glitch.colSliceIntensity", "%.3f", ms);
    ImGui::SliderFloat("Burst Freq##colslice", &g->colSliceBurstFreq, 0.5f,
                       20.0f, "%.1f Hz");
    ImGui::SliderFloat("Burst Power##colslice", &g->colSliceBurstPower, 1.0f,
                       15.0f, "%.1f");
    ImGui::SliderFloat("Row Height##colslice", &g->colSliceRows, 8.0f, 128.0f,
                       "%.0f");
  }
}

static void DrawGlitchDiagonalBands(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Diagonal Bands");
  ImGui::Checkbox("Enabled##diagbands", &g->diagonalBandsEnabled);
  if (g->diagonalBandsEnabled) {
    ImGui::SliderFloat("Band Count##diagbands", &g->diagonalBandCount, 2.0f,
                       32.0f, "%.0f");
    ModulatableSlider("Displace##diagbands", &g->diagonalBandDisplace,
                      "glitch.diagonalBandDisplace", "%.3f", ms);
    ImGui::SliderFloat("Speed##diagbands", &g->diagonalBandSpeed, 0.0f, 10.0f,
                       "%.1f");
  }
}

static void DrawGlitchBlockMask(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Block Mask");
  ImGui::Checkbox("Enabled##blockmask", &g->blockMaskEnabled);
  if (g->blockMaskEnabled) {
    ModulatableSlider("Intensity##blockmask", &g->blockMaskIntensity,
                      "glitch.blockMaskIntensity", "%.2f", ms);
    ImGui::SliderInt("Min Size##blockmask", &g->blockMaskMinSize, 1, 10);
    ImGui::SliderInt("Max Size##blockmask", &g->blockMaskMaxSize, 5, 20);
    float tint[3] = {g->blockMaskTintR, g->blockMaskTintG, g->blockMaskTintB};
    if (ImGui::ColorEdit3("Tint##blockmask", tint)) {
      g->blockMaskTintR = tint[0];
      g->blockMaskTintG = tint[1];
      g->blockMaskTintB = tint[2];
    }
  }
}

static void DrawGlitchTemporal(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Temporal");
  ImGui::Checkbox("Enabled##temporal", &g->temporalJitterEnabled);
  if (g->temporalJitterEnabled) {
    ModulatableSlider("Amount##temporal", &g->temporalJitterAmount,
                      "glitch.temporalJitterAmount", "%.3f", ms);
    ModulatableSlider("Gate##temporal", &g->temporalJitterGate,
                      "glitch.temporalJitterGate", "%.2f", ms);
  }
}

static void DrawGlitchBlockMultiply(GlitchConfig *g, const ModSources *ms) {
  ImGui::SeparatorText("Block Multiply");
  ImGui::Checkbox("Enabled##blockmultiply", &g->blockMultiplyEnabled);
  if (g->blockMultiplyEnabled) {
    ModulatableSlider("Block Size##blockmultiply", &g->blockMultiplySize,
                      "glitch.blockMultiplySize", "%.1f", ms);
    ModulatableSlider("Distortion##blockmultiply", &g->blockMultiplyControl,
                      "glitch.blockMultiplyControl", "%.3f", ms);
    ImGui::SliderInt("Iterations##blockmultiply", &g->blockMultiplyIterations,
                     1, 8);
    ModulatableSlider("Intensity##blockmultiply", &g->blockMultiplyIntensity,
                      "glitch.blockMultiplyIntensity", "%.2f", ms);
  }
}

static void DrawGlitchParams(EffectConfig *e, const ModSources *ms,
                             ImU32 glow) {
  (void)glow;
  GlitchConfig *g = &e->glitch;

  DrawGlitchAnalog(g, ms);
  DrawGlitchDigital(g, ms);
  DrawGlitchVHS(g);
  DrawGlitchDatamosh(g, ms);
  DrawGlitchSlice(g, ms);
  DrawGlitchDiagonalBands(g, ms);
  DrawGlitchBlockMask(g, ms);
  DrawGlitchTemporal(g, ms);
  DrawGlitchBlockMultiply(g, ms);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Overlay");
  ImGui::SliderFloat("Scanlines##glitch", &g->scanlineAmount, 0.0f, 0.5f,
                     "%.2f");
  ImGui::SliderFloat("Noise##glitch", &g->noiseAmount, 0.0f, 0.3f, "%.2f");
}

GlitchEffect *GetGlitchEffect(PostEffect *pe) {
  return (GlitchEffect *)pe->effectStates[TRANSFORM_GLITCH];
}

void SetupGlitch(PostEffect *pe) {
  GlitchEffectSetup(GetGlitchEffect(pe), &pe->effects.glitch,
                    pe->currentDeltaTime);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_GLITCH, Glitch, glitch, "Glitch", "RET", 6,
                EFFECT_FLAG_NONE, SetupGlitch, NULL, DrawGlitchParams)
// clang-format on
