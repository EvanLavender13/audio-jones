// Palette quantization effect module implementation

#include "palette_quantization.h"

#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_descriptor.h"
#include "render/post_effect.h"
#include <stddef.h>

#include "imgui.h"
#include "ui/modulatable_slider.h"

bool PaletteQuantizationEffectInit(PaletteQuantizationEffect *e) {
  e->shader = LoadShader(NULL, "shaders/palette_quantization.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->colorLevelsLoc = GetShaderLocation(e->shader, "colorLevels");
  e->ditherStrengthLoc = GetShaderLocation(e->shader, "ditherStrength");
  e->bayerSizeLoc = GetShaderLocation(e->shader, "bayerSize");

  return true;
}

void PaletteQuantizationEffectSetup(PaletteQuantizationEffect *e,
                                    const PaletteQuantizationConfig *cfg) {
  SetShaderValue(e->shader, e->colorLevelsLoc, &cfg->colorLevels,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->ditherStrengthLoc, &cfg->ditherStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bayerSizeLoc, &cfg->bayerSize,
                 SHADER_UNIFORM_INT);
}

void PaletteQuantizationEffectUninit(PaletteQuantizationEffect *e) {
  UnloadShader(e->shader);
}

PaletteQuantizationConfig PaletteQuantizationConfigDefault(void) {
  return PaletteQuantizationConfig{};
}

void PaletteQuantizationRegisterParams(PaletteQuantizationConfig *cfg) {
  ModEngineRegisterParam("paletteQuantization.colorLevels", &cfg->colorLevels,
                         2.0f, 16.0f);
  ModEngineRegisterParam("paletteQuantization.ditherStrength",
                         &cfg->ditherStrength, 0.0f, 1.0f);
}

// === UI ===

static void DrawPaletteQuantizationParams(EffectConfig *e, const ModSources *ms,
                                          ImU32 glow) {
  (void)glow;
  PaletteQuantizationConfig *pq = &e->paletteQuantization;

  ModulatableSlider("Color Levels##palettequant", &pq->colorLevels,
                    "paletteQuantization.colorLevels", "%.0f", ms);
  ModulatableSlider("Dither##palettequant", &pq->ditherStrength,
                    "paletteQuantization.ditherStrength", "%.2f", ms);

  const char *bayerSizeNames[] = {"4x4 (Coarse)", "8x8 (Fine)"};
  int bayerIndex = (pq->bayerSize == 4) ? 0 : 1;
  if (ImGui::Combo("Pattern##palettequant", &bayerIndex, bayerSizeNames, 2)) {
    pq->bayerSize = (bayerIndex == 0) ? 4 : 8;
  }
}

void SetupPaletteQuantization(PostEffect *pe) {
  PaletteQuantizationEffectSetup(&pe->paletteQuantization,
                                 &pe->effects.paletteQuantization);
}

// clang-format off
REGISTER_EFFECT(TRANSFORM_PALETTE_QUANTIZATION, PaletteQuantization,
                paletteQuantization, "Palette Quantization", "COL", 8,
                EFFECT_FLAG_NONE, SetupPaletteQuantization, NULL,
                DrawPaletteQuantizationParams)
// clang-format on
