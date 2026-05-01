// Dancing lines effect module implementation
// Lissajous-driven line snaps on a clock and leaves a fading trail of past
// poses with per-echo gradient color and FFT band brightness

#include "dancing_lines.h"
#include "audio/audio.h"
#include "automation/mod_sources.h"
#include "automation/modulation_engine.h"
#include "config/constants.h"
#include "config/effect_config.h"
#include "config/effect_descriptor.h"
#include "imgui.h"
#include "render/blend_compositor.h"
#include "render/color_lut.h"
#include "render/post_effect.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/ui_units.h"
#include <stddef.h>

bool DancingLinesEffectInit(DancingLinesEffect *e,
                            const DancingLinesConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/dancing_lines.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->phaseLoc = GetShaderLocation(e->shader, "phase");
  e->amplitudeLoc = GetShaderLocation(e->shader, "amplitude");
  e->freqX1Loc = GetShaderLocation(e->shader, "freqX1");
  e->freqY1Loc = GetShaderLocation(e->shader, "freqY1");
  e->freqX2Loc = GetShaderLocation(e->shader, "freqX2");
  e->freqY2Loc = GetShaderLocation(e->shader, "freqY2");
  e->offsetX2Loc = GetShaderLocation(e->shader, "offsetX2");
  e->offsetY2Loc = GetShaderLocation(e->shader, "offsetY2");
  e->accumTimeLoc = GetShaderLocation(e->shader, "accumTime");
  e->snapRateLoc = GetShaderLocation(e->shader, "snapRate");
  e->trailLengthLoc = GetShaderLocation(e->shader, "trailLength");
  e->trailFadeLoc = GetShaderLocation(e->shader, "trailFade");
  e->colorPhaseStepLoc = GetShaderLocation(e->shader, "colorPhaseStep");
  e->lineThicknessLoc = GetShaderLocation(e->shader, "lineThickness");
  e->endpointOffsetLoc = GetShaderLocation(e->shader, "endpointOffset");
  e->baseBrightLoc = GetShaderLocation(e->shader, "baseBright");
  e->glowIntensityLoc = GetShaderLocation(e->shader, "glowIntensity");
  e->baseFreqLoc = GetShaderLocation(e->shader, "baseFreq");
  e->maxFreqLoc = GetShaderLocation(e->shader, "maxFreq");
  e->gainLoc = GetShaderLocation(e->shader, "gain");
  e->curveLoc = GetShaderLocation(e->shader, "curve");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");

  e->gradientLUT = ColorLUTInit(&cfg->gradient);
  if (e->gradientLUT == NULL) {
    UnloadShader(e->shader);
    return false;
  }

  e->accumTime = 0.0f;

  return true;
}

void DancingLinesEffectSetup(DancingLinesEffect *e, DancingLinesConfig *cfg,
                             float deltaTime, const Texture2D &fftTexture) {
  cfg->lissajous.phase += cfg->lissajous.motionSpeed * deltaTime;
  e->accumTime += deltaTime;
  // Wrap to prevent float precision loss at large values
  if (e->accumTime > 1000.0f) {
    e->accumTime -= 1000.0f;
  }

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->phaseLoc, &cfg->lissajous.phase,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->amplitudeLoc, &cfg->lissajous.amplitude,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqX1Loc, &cfg->lissajous.freqX1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqY1Loc, &cfg->lissajous.freqY1,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqX2Loc, &cfg->lissajous.freqX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->freqY2Loc, &cfg->lissajous.freqY2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetX2Loc, &cfg->lissajous.offsetX2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->offsetY2Loc, &cfg->lissajous.offsetY2,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->accumTimeLoc, &e->accumTime,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->snapRateLoc, &cfg->snapRate,
                 SHADER_UNIFORM_FLOAT);
  int trailLength = cfg->trailLength < 1 ? 1 : cfg->trailLength;
  SetShaderValue(e->shader, e->trailLengthLoc, &trailLength,
                 SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->trailFadeLoc, &cfg->trailFade,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->colorPhaseStepLoc, &cfg->colorPhaseStep,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->lineThicknessLoc, &cfg->lineThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->endpointOffsetLoc, &cfg->endpointOffset,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->glowIntensityLoc, &cfg->glowIntensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
}

void DancingLinesEffectUninit(DancingLinesEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void DancingLinesRegisterParams(DancingLinesConfig *cfg) {
  ModEngineRegisterParam("dancingLines.lissajous.amplitude",
                         &cfg->lissajous.amplitude, 0.05f, 2.0f);
  ModEngineRegisterParam("dancingLines.lissajous.motionSpeed",
                         &cfg->lissajous.motionSpeed, 0.0f, 5.0f);
  ModEngineRegisterParam("dancingLines.lissajous.offsetX2",
                         &cfg->lissajous.offsetX2, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("dancingLines.lissajous.offsetY2",
                         &cfg->lissajous.offsetY2, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("dancingLines.snapRate", &cfg->snapRate, 1.0f, 60.0f);
  ModEngineRegisterParam("dancingLines.trailFade", &cfg->trailFade, 0.5f,
                         0.99f);
  ModEngineRegisterParam("dancingLines.colorPhaseStep", &cfg->colorPhaseStep,
                         0.05f, 1.0f);
  ModEngineRegisterParam("dancingLines.lineThickness", &cfg->lineThickness,
                         0.0005f, 0.02f);
  ModEngineRegisterParam("dancingLines.endpointOffset", &cfg->endpointOffset,
                         0.1f, PI_F);
  ModEngineRegisterParam("dancingLines.baseBright", &cfg->baseBright, 0.0f,
                         1.0f);
  ModEngineRegisterParam("dancingLines.glowIntensity", &cfg->glowIntensity,
                         0.0f, 3.0f);
  ModEngineRegisterParam("dancingLines.baseFreq", &cfg->baseFreq, 27.5f,
                         440.0f);
  ModEngineRegisterParam("dancingLines.maxFreq", &cfg->maxFreq, 1000.0f,
                         16000.0f);
  ModEngineRegisterParam("dancingLines.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("dancingLines.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("dancingLines.blendIntensity", &cfg->blendIntensity,
                         0.0f, 5.0f);
}

DancingLinesEffect *GetDancingLinesEffect(PostEffect *pe) {
  return (DancingLinesEffect *)pe->effectStates[TRANSFORM_DANCING_LINES_BLEND];
}

void SetupDancingLines(PostEffect *pe) {
  DancingLinesEffectSetup(GetDancingLinesEffect(pe), &pe->effects.dancingLines,
                          pe->currentDeltaTime, pe->fftTexture);
}

void SetupDancingLinesBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.dancingLines.blendIntensity,
                       pe->effects.dancingLines.blendMode);
}

// === UI ===

static void DrawDancingLinesParams(EffectConfig *e,
                                   const ModSources *modSources,
                                   ImU32 categoryGlow) {
  (void)categoryGlow;
  DancingLinesConfig *cfg = &e->dancingLines;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##dancinglines", &cfg->baseFreq,
                    "dancingLines.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##dancinglines", &cfg->maxFreq,
                    "dancingLines.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##dancinglines", &cfg->gain, "dancingLines.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##dancinglines", &cfg->curve, "dancingLines.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##dancinglines", &cfg->baseBright,
                    "dancingLines.baseBright", "%.2f", modSources);

  // Trail
  ImGui::SeparatorText("Trail");
  ModulatableSlider("Snap Rate (Hz)##dancinglines", &cfg->snapRate,
                    "dancingLines.snapRate", "%.1f", modSources);
  ImGui::SliderInt("Trail Length##dancinglines", &cfg->trailLength, 1, 32);
  ModulatableSlider("Trail Fade##dancinglines", &cfg->trailFade,
                    "dancingLines.trailFade", "%.2f", modSources);
  ModulatableSlider("Color Phase Step##dancinglines", &cfg->colorPhaseStep,
                    "dancingLines.colorPhaseStep", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Line Thickness##dancinglines", &cfg->lineThickness,
                    "dancingLines.lineThickness", "%.4f", modSources);
  ModulatableSlider("Endpoint Offset##dancinglines", &cfg->endpointOffset,
                    "dancingLines.endpointOffset", "%.3f", modSources);

  // Lissajous
  ImGui::SeparatorText("Lissajous");
  DrawLissajousControls(&cfg->lissajous, "dancinglines",
                        "dancingLines.lissajous", modSources, 10.0f);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##dancinglines", &cfg->glowIntensity,
                    "dancingLines.glowIntensity", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(dancingLines)
REGISTER_GENERATOR(TRANSFORM_DANCING_LINES_BLEND, DancingLines, dancingLines,
                   "Dancing Lines", SetupDancingLinesBlend, SetupDancingLines, 11,
                   DrawDancingLinesParams, DrawOutput_dancingLines)
// clang-format on
