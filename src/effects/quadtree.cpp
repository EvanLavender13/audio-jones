// Quadtree generator - recursive subdivision around moving Lissajous source
// points, depth/hash colored via gradient LUT and FFT band lookup

#include "quadtree.h"
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
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool QuadtreeEffectInit(QuadtreeEffect *e, const QuadtreeConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/quadtree.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->sourcesLoc = GetShaderLocation(e->shader, "sources");
  e->pointCountLoc = GetShaderLocation(e->shader, "pointCount");
  e->maxIterationsLoc = GetShaderLocation(e->shader, "maxIterations");
  e->lineWidthLoc = GetShaderLocation(e->shader, "lineWidth");
  e->cellFillAmountLoc = GetShaderLocation(e->shader, "cellFillAmount");
  e->colorModeLoc = GetShaderLocation(e->shader, "colorMode");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
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
  return true;
}

void QuadtreeEffectSetup(QuadtreeEffect *e, QuadtreeConfig *cfg,
                         float deltaTime, const Texture2D &fftTexture) {
  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);

  float sources[16];
  int count = cfg->pointCount;
  if (count < 1) {
    count = 1;
  } else if (count > 8) {
    count = 8;
  }
  DualLissajousUpdateMulti(&cfg->lissajous, deltaTime, 0.5f, 0.5f, count,
                           sources);
  SetShaderValueV(e->shader, e->sourcesLoc, sources, SHADER_UNIFORM_VEC2,
                  count);
  SetShaderValue(e->shader, e->pointCountLoc, &count, SHADER_UNIFORM_INT);

  SetShaderValue(e->shader, e->maxIterationsLoc, &cfg->maxIterations,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->lineWidthLoc, &cfg->lineWidth,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->cellFillAmountLoc, &cfg->cellFillAmount,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorModeLoc, &cfg->colorMode,
                 SHADER_UNIFORM_INT);

  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);
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
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void QuadtreeEffectUninit(QuadtreeEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void QuadtreeRegisterParams(QuadtreeConfig *cfg) {
  ModEngineRegisterParam("quadtree.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("quadtree.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("quadtree.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("quadtree.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("quadtree.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("quadtree.lineWidth", &cfg->lineWidth, 0.5f, 4.0f);
  ModEngineRegisterParam("quadtree.cellFillAmount", &cfg->cellFillAmount, 0.0f,
                         1.0f);
  ModEngineRegisterParam("quadtree.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.0f, 0.5f);
  ModEngineRegisterParam("quadtree.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("quadtree.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

QuadtreeEffect *GetQuadtreeEffect(PostEffect *pe) {
  return (QuadtreeEffect *)pe->effectStates[TRANSFORM_QUADTREE];
}

void SetupQuadtree(PostEffect *pe) {
  QuadtreeEffectSetup(GetQuadtreeEffect(pe), &pe->effects.quadtree,
                      pe->currentDeltaTime, pe->fftTexture);
}

void SetupQuadtreeBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.quadtree.blendIntensity,
                       pe->effects.quadtree.blendMode);
}

// === UI ===

static void DrawQuadtreeParams(EffectConfig *e, const ModSources *ms,
                               ImU32 glow) {
  (void)glow;
  QuadtreeConfig *cfg = &e->quadtree;

  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##quadtree", &cfg->baseFreq,
                    "quadtree.baseFreq", "%.1f", ms);
  ModulatableSlider("Max Freq (Hz)##quadtree", &cfg->maxFreq,
                    "quadtree.maxFreq", "%.0f", ms);
  ModulatableSlider("Gain##quadtree", &cfg->gain, "quadtree.gain", "%.1f", ms);
  ModulatableSlider("Contrast##quadtree", &cfg->curve, "quadtree.curve", "%.2f",
                    ms);
  ModulatableSlider("Base Bright##quadtree", &cfg->baseBright,
                    "quadtree.baseBright", "%.2f", ms);

  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Iterations##quadtree", &cfg->maxIterations, 1, 8);
  ModulatableSlider("Line Width##quadtree", &cfg->lineWidth,
                    "quadtree.lineWidth", "%.2f", ms);
  ModulatableSlider("Fill##quadtree", &cfg->cellFillAmount,
                    "quadtree.cellFillAmount", "%.2f", ms);

  ImGui::SeparatorText("Sources");
  ImGui::SliderInt("Source Count##quadtree", &cfg->pointCount, 1, 8);
  DrawLissajousControls(&cfg->lissajous, "quadtree_liss", "quadtree.lissajous",
                        ms, 5.0f);

  ImGui::SeparatorText("Color");
  ImGui::Combo("Color Mode##quadtree", &cfg->colorMode, "Depth\0Hash\0");
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(quadtree)
REGISTER_GENERATOR(TRANSFORM_QUADTREE, Quadtree, quadtree,
                   "Quadtree", SetupQuadtreeBlend,
                   SetupQuadtree, 12, DrawQuadtreeParams, DrawOutput_quadtree)
// clang-format on
