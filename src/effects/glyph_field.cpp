// Glyph field effect module implementation
// Renders scrolling character grids with layered depth and LCD sub-pixel
// overlay

#include "glyph_field.h"
#include "audio/audio.h"
#include "automation/modulation_engine.h"
#include "render/color_lut.h"
#include <stddef.h>

static void CacheLocations(GlyphFieldEffect *e) {
  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->gridSizeLoc = GetShaderLocation(e->shader, "gridSize");
  e->layerCountLoc = GetShaderLocation(e->shader, "layerCount");
  e->layerScaleSpreadLoc = GetShaderLocation(e->shader, "layerScaleSpread");
  e->layerSpeedSpreadLoc = GetShaderLocation(e->shader, "layerSpeedSpread");
  e->layerOpacityLoc = GetShaderLocation(e->shader, "layerOpacity");
  e->scrollDirectionLoc = GetShaderLocation(e->shader, "scrollDirection");
  e->scrollTimeLoc = GetShaderLocation(e->shader, "scrollTime");
  e->flutterAmountLoc = GetShaderLocation(e->shader, "flutterAmount");
  e->flutterTimeLoc = GetShaderLocation(e->shader, "flutterTime");
  e->waveAmplitudeLoc = GetShaderLocation(e->shader, "waveAmplitude");
  e->waveFreqLoc = GetShaderLocation(e->shader, "waveFreq");
  e->waveTimeLoc = GetShaderLocation(e->shader, "waveTime");
  e->driftAmountLoc = GetShaderLocation(e->shader, "driftAmount");
  e->driftTimeLoc = GetShaderLocation(e->shader, "driftTime");
  e->bandDistortionLoc = GetShaderLocation(e->shader, "bandDistortion");
  e->inversionRateLoc = GetShaderLocation(e->shader, "inversionRate");
  e->inversionTimeLoc = GetShaderLocation(e->shader, "inversionTime");
  e->lcdModeLoc = GetShaderLocation(e->shader, "lcdMode");
  e->lcdFreqLoc = GetShaderLocation(e->shader, "lcdFreq");
  e->fontAtlasLoc = GetShaderLocation(e->shader, "fontAtlas");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->numOctavesLoc = GetShaderLocation(e->shader, "numOctaves");
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
  e->flutterTime = 0.0f;
  e->waveTime = 0.0f;
  e->driftTime = 0.0f;
  e->inversionTime = 0.0f;
  e->stutterTime = 0.0f;

  return true;
}

static void BindUniforms(GlyphFieldEffect *e, const GlyphFieldConfig *cfg,
                         Texture2D fftTexture) {
  float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
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
  SetShaderValue(e->shader, e->scrollDirectionLoc, &cfg->scrollDirection,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->scrollTimeLoc, &e->scrollTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flutterAmountLoc, &cfg->flutterAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->flutterTimeLoc, &e->flutterTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveAmplitudeLoc, &cfg->waveAmplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveFreqLoc, &cfg->waveFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->waveTimeLoc, &e->waveTime, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftAmountLoc, &cfg->driftAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->driftTimeLoc, &e->driftTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bandDistortionLoc, &cfg->bandDistortion,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->inversionRateLoc, &cfg->inversionRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->inversionTimeLoc, &e->inversionTime,
                 SHADER_UNIFORM_FLOAT);

  int lcdModeInt = cfg->lcdMode ? 1 : 0;
  SetShaderValue(e->shader, e->lcdModeLoc, &lcdModeInt, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lcdFreqLoc, &cfg->lcdFreq, SHADER_UNIFORM_FLOAT);

  SetShaderValueTexture(e->shader, e->fontAtlasLoc, e->fontAtlas);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  int numOctavesInt = (int)cfg->numOctaves;
  SetShaderValue(e->shader, e->numOctavesLoc, &numOctavesInt,
                 SHADER_UNIFORM_INT);
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
                           float deltaTime, Texture2D fftTexture) {
  e->scrollTime += cfg->scrollSpeed * deltaTime;
  e->flutterTime += cfg->flutterSpeed * deltaTime;
  e->waveTime += cfg->waveSpeed * deltaTime;
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

GlyphFieldConfig GlyphFieldConfigDefault(void) { return GlyphFieldConfig{}; }

void GlyphFieldRegisterParams(GlyphFieldConfig *cfg) {
  ModEngineRegisterParam("glyphField.gridSize", &cfg->gridSize, 8.0f, 64.0f);
  ModEngineRegisterParam("glyphField.layerScaleSpread", &cfg->layerScaleSpread,
                         0.5f, 2.0f);
  ModEngineRegisterParam("glyphField.layerSpeedSpread", &cfg->layerSpeedSpread,
                         0.5f, 2.0f);
  ModEngineRegisterParam("glyphField.layerOpacity", &cfg->layerOpacity, 0.1f,
                         1.0f);
  ModEngineRegisterParam("glyphField.scrollSpeed", &cfg->scrollSpeed, 0.0f,
                         2.0f);
  ModEngineRegisterParam("glyphField.flutterAmount", &cfg->flutterAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("glyphField.flutterSpeed", &cfg->flutterSpeed, 0.1f,
                         10.0f);
  ModEngineRegisterParam("glyphField.waveAmplitude", &cfg->waveAmplitude, 0.0f,
                         0.5f);
  ModEngineRegisterParam("glyphField.waveFreq", &cfg->waveFreq, 1.0f, 20.0f);
  ModEngineRegisterParam("glyphField.waveSpeed", &cfg->waveSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("glyphField.driftAmount", &cfg->driftAmount, 0.0f,
                         0.5f);
  ModEngineRegisterParam("glyphField.driftSpeed", &cfg->driftSpeed, 0.1f, 5.0f);
  ModEngineRegisterParam("glyphField.bandDistortion", &cfg->bandDistortion,
                         0.0f, 1.0f);
  ModEngineRegisterParam("glyphField.inversionRate", &cfg->inversionRate, 0.0f,
                         1.0f);
  ModEngineRegisterParam("glyphField.inversionSpeed", &cfg->inversionSpeed,
                         0.0f, 2.0f);
  ModEngineRegisterParam("glyphField.lcdFreq", &cfg->lcdFreq, 0.1f, 6.283f);
  ModEngineRegisterParam("glyphField.baseFreq", &cfg->baseFreq, 20.0f, 200.0f);
  ModEngineRegisterParam("glyphField.numOctaves", &cfg->numOctaves, 1.0f, 8.0f);
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
