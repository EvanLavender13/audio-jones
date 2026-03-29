// Glyph field effect module implementation
// Renders scrolling character grids with layered depth

#include "glyph_field.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/blend_mode.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include <stddef.h>

static void CacheLocations(GlyphFieldEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gridSizeLoc = GetShaderLocation(e->shader, "gridSize");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->layerScaleSpreadLoc = GetShaderLocation(e->shader, "layerScaleSpread");
  e->layerSpeedSpreadLoc = GetShaderLocation(e->shader, "layerSpeedSpread");
  e->layerOpacityLoc = GetShaderLocation(e->shader, "layerOpacity");
  e->bandStrengthLoc = GetShaderLocation(e->shader, "bandStrength");
  e->scrollDirectionLoc = GetShaderLocation(e->shader, "scrollDirection");
  e->scrollTimeLoc = GetShaderLocation(e->shader, "scrollTime");
  e->charAmountLoc = GetShaderLocation(e->shader, "charAmount");
  e->charTimeLoc = GetShaderLocation(e->shader, "charTime");
  e->driftAmountLoc = GetShaderLocation(e->shader, "driftAmount");
  e->driftTimeLoc = GetShaderLocation(e->shader, "driftTime");
  e->inversionRateLoc = GetShaderLocation(e->shader, "inversionRate");
  e->inversionTimeLoc = GetShaderLocation(e->shader, "inversionTime");
  e->fontAtlasLoc = GetShaderLocation(e->shader, "fontAtlas");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->stutterAmountLoc = GetShaderLocation(e->shader, "stutterAmount");
  e->stutterTimeLoc = GetShaderLocation(e->shader, "stutterTime");
  e->stutterDiscreteLoc = GetShaderLocation(e->shader, "stutterDiscrete");
}

bool GlyphFieldEffectInit(GlyphFieldEffect *e, const GlyphFieldConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/glyph_field.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->fontAtlas = LoadTexture("fonts/font_atlas.png");
  if (e->fontAtlas.id == 0) {
    UnloadShader(e->shader);
    return false;
  }
  SetTextureFilter(e->fontAtlas, TEXTURE_FILTER_BILINEAR);
  SetTextureWrap(e->fontAtlas, TEXTURE_WRAP_REPEAT);

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadTexture(e->fontAtlas);
    UnloadShader(e->shader);
    return false;
  }

  CacheLocations(e);

  e->scrollTime = 0.0f;
  e->charTime = 0.0f;
  e->driftTime = 0.0f;
  e->inversionTime = 0.0f;
  e->stutterTime = 0.0f;

  return true;
}

static void BindUniforms(GlyphFieldEffect *e, const GlyphFieldConfig *cfg,
                         const Texture2D &fftTexture) {
  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  SetShaderValue(e->shader, e->gridSizeLoc, &cfg->gridSize,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layerCountLoc, &cfg->layerCount,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->layerScaleSpreadLoc, &cfg->layerScaleSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layerSpeedSpreadLoc, &cfg->layerSpeedSpread,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->layerOpacityLoc, &cfg->layerOpacity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bandStrengthLoc, &cfg->bandStrength,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->scrollDirectionLoc, &cfg->scrollDirection,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scrollTimeLoc, &e->scrollTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->charAmountLoc, &cfg->charAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->charTimeLoc, &e->charTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftAmountLoc, &cfg->driftAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftTimeLoc, &e->driftTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->inversionRateLoc, &cfg->inversionRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->inversionTimeLoc, &e->inversionTime,
                 SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->fontAtlasLoc, e->fontAtlas);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stutterAmountLoc, &cfg->stutterAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stutterTimeLoc, &e->stutterTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->stutterDiscreteLoc, &cfg->stutterDiscrete,
                 SHADER_UNIFORM_FLOAT);
}

void GlyphFieldEffectSetup(GlyphFieldEffect *e, const GlyphFieldConfig *cfg,
                           float deltaTime, const Texture2D &fftTexture) {
  e->scrollTime += cfg->scrollSpeed * deltaTime;
  e->charTime += cfg->charSpeed * deltaTime;
  e->driftTime += cfg->driftSpeed * deltaTime;
  e->inversionTime += cfg->inversionSpeed * deltaTime;
  e->stutterTime += cfg->stutterSpeed * deltaTime;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);
  BindUniforms(e, cfg, fftTexture);
}

void GlyphFieldEffectUninit(GlyphFieldEffect *e) {
  UnloadShader(e->shader);
  UnloadTexture(e->fontAtlas);
  ColorLUTUninit(e->gradientLUT);
}

void GlyphFieldRegisterParams(GlyphFieldConfig *cfg) {
  ModEngineRegisterParam("glyphField.gridSize", &cfg->gridSize, 8.0f, 64.0f);
  ModEngineRegisterParam("glyphField.layerScaleSpread", &cfg->layerScaleSpread,
                         0.5f, 2.0f);
  ModEngineRegisterParam("glyphField.layerSpeedSpread", &cfg->layerSpeedSpread,
                         0.5f, 2.0f);
  ModEngineRegisterParam("glyphField.layerOpacity", &cfg->layerOpacity, 0.1f,
                         1.0f);
  ModEngineRegisterParam("glyphField.bandStrength", &cfg->bandStrength, 0.0f,
                         1.0f);
  ModEngineRegisterParam("glyphField.scrollSpeed", &cfg->scrollSpeed, 0.0f,
                         2.0f);
  ModEngineRegisterParam("glyphField.charAmount", &cfg->charAmount, 0.0f, 1.0f);
  ModEngineRegisterParam("glyphField.charSpeed", &cfg->charSpeed, 0.1f, 10.0f);
  ModEngineRegisterParam("glyphField.inversionRate", &cfg->inversionRate, 0.0f,
                         1.0f);
  ModEngineRegisterParam("glyphField.inversionSpeed", &cfg->inversionSpeed,
                         0.0f, 2.0f);
  ModEngineRegisterParam("glyphField.driftAmount", &cfg->driftAmount, 0.0f,
                         0.5f);
  ModEngineRegisterParam("glyphField.driftSpeed", &cfg->driftSpeed, 0.1f, 5.0f);
  ModEngineRegisterParam("glyphField.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("glyphField.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("glyphField.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("glyphField.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("glyphField.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("glyphField.stutterAmount", &cfg->stutterAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("glyphField.stutterSpeed", &cfg->stutterSpeed, 0.1f,
                         5.0f);
  ModEngineRegisterParam("glyphField.stutterDiscrete", &cfg->stutterDiscrete,
                         0.0f, 1.0f);
  ModEngineRegisterParam("glyphField.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

void SetupGlyphField(PostEffect *pe) {
  GlyphFieldEffectSetup(&pe->glyphField, &pe->effects.glyphField,
                        pe->currentDeltaTime, pe->fftTexture);
}

void SetupGlyphFieldBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.glyphField.blendIntensity,
                       pe->effects.glyphField.blendMode);
}

// === UI ===

static void DrawGlyphFieldParams(EffectConfig *e, const ModSources *modSources,
                                 ImU32 categoryGlow) {
  (void)categoryGlow;
  GlyphFieldConfig *c = &e->glyphField;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##glyphfield", &c->baseFreq,
                    "glyphField.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##glyphfield", &c->maxFreq,
                    "glyphField.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##glyphfield", &c->gain, "glyphField.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##glyphfield", &c->curve, "glyphField.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##glyphfield", &c->baseBright,
                    "glyphField.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Grid Size##glyphfield", &c->gridSize,
                    "glyphField.gridSize", "%.1f", modSources);
  ImGui::SliderInt("Layers##glyphfield", &c->layerCount, 1, 4);
  ModulatableSlider("Layer Scale##glyphfield", &c->layerScaleSpread,
                    "glyphField.layerScaleSpread", "%.2f", modSources);
  ModulatableSlider("Layer Speed##glyphfield", &c->layerSpeedSpread,
                    "glyphField.layerSpeedSpread", "%.2f", modSources);
  ModulatableSlider("Layer Opacity##glyphfield", &c->layerOpacity,
                    "glyphField.layerOpacity", "%.2f", modSources);
  ModulatableSlider("Band Strength##glyphfield", &c->bandStrength,
                    "glyphField.bandStrength", "%.2f", modSources);

  // Scroll
  ImGui::SeparatorText("Scroll");
  ImGui::Combo("Scroll Dir##glyphfield", &c->scrollDirection,
               "Horizontal\0Vertical\0Radial\0");
  ModulatableSlider("Scroll Speed##glyphfield", &c->scrollSpeed,
                    "glyphField.scrollSpeed", "%.2f", modSources);

  // Stutter
  ImGui::SeparatorText("Stutter");
  ModulatableSlider("Stutter##glyphfield", &c->stutterAmount,
                    "glyphField.stutterAmount", "%.2f", modSources);
  ModulatableSlider("Stutter Speed##glyphfield", &c->stutterSpeed,
                    "glyphField.stutterSpeed", "%.2f", modSources);
  ModulatableSlider("Discrete##glyphfield", &c->stutterDiscrete,
                    "glyphField.stutterDiscrete", "%.2f", modSources);

  // Character
  ImGui::SeparatorText("Character");
  ModulatableSlider("Char Cycle##glyphfield", &c->charAmount,
                    "glyphField.charAmount", "%.2f", modSources);
  ModulatableSlider("Char Speed##glyphfield", &c->charSpeed,
                    "glyphField.charSpeed", "%.1f", modSources);
  ModulatableSlider("Inversion##glyphfield", &c->inversionRate,
                    "glyphField.inversionRate", "%.2f", modSources);
  ModulatableSlider("Inversion Speed##glyphfield", &c->inversionSpeed,
                    "glyphField.inversionSpeed", "%.2f", modSources);

  // Drift
  ImGui::SeparatorText("Drift");
  ModulatableSlider("Drift##glyphfield", &c->driftAmount,
                    "glyphField.driftAmount", "%.3f", modSources);
  ModulatableSlider("Drift Speed##glyphfield", &c->driftSpeed,
                    "glyphField.driftSpeed", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(glyphField)
REGISTER_GENERATOR(TRANSFORM_GLYPH_FIELD_BLEND, GlyphField, glyphField,
                   "Glyph Field", SetupGlyphFieldBlend, SetupGlyphField, 12, DrawGlyphFieldParams, DrawOutput_glyphField)
// clang-format on
