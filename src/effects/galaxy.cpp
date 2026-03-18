// Galaxy generator effect module implementation
// Tilted multi-ring spiral galaxy with dust lanes, star points, and central
// bulge glow - each ring driven by FFT semitone energy

#include "galaxy.h"
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

bool GalaxyEffectInit(GalaxyEffect *e, const GalaxyConfig *cfg) {
  e->shader = LoadShader(NULL, "shaders/galaxy.fs");
  if (e->shader.id == 0) {
    return false;
  }

  e->resolutionLoc = GetShaderLocation(e->shader, "resolution");
  e->timeLoc = GetShaderLocation(e->shader, "time");
  e->fftTextureLoc = GetShaderLocation(e->shader, "fftTexture");
  e->sampleRateLoc = GetShaderLocation(e->shader, "sampleRate");
  e->gradientLUTLoc = GetShaderLocation(e->shader, "gradientLUT");
  e->layersLoc = GetShaderLocation(e->shader, "layers");
  e->twistLoc = GetShaderLocation(e->shader, "twist");
  e->innerStretchLoc = GetShaderLocation(e->shader, "innerStretch");
  e->ringWidthLoc = GetShaderLocation(e->shader, "ringWidth");
  e->diskThicknessLoc = GetShaderLocation(e->shader, "diskThickness");
  e->tiltLoc = GetShaderLocation(e->shader, "tilt");
  e->rotationLoc = GetShaderLocation(e->shader, "rotation");

  e->dustContrastLoc = GetShaderLocation(e->shader, "dustContrast");
  e->starDensityLoc = GetShaderLocation(e->shader, "starDensity");
  e->starBrightLoc = GetShaderLocation(e->shader, "starBright");
  e->bulgeSizeLoc = GetShaderLocation(e->shader, "bulgeSize");
  e->bulgeBrightLoc = GetShaderLocation(e->shader, "bulgeBright");
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

  e->time = 0.0f;

  return true;
}

void GalaxyEffectSetup(GalaxyEffect *e, const GalaxyConfig *cfg,
                       float deltaTime, const Texture2D &fftTexture) {
  e->time += deltaTime * cfg->orbitSpeed;

  ColorLUTUpdate(e->gradientLUT, &cfg->gradient);

  const float resolution[2] = {(float)GetScreenWidth(),
                               (float)GetScreenHeight()};
  SetShaderValue(e->shader, e->resolutionLoc, resolution, SHADER_UNIFORM_VEC2);
  SetShaderValue(e->shader, e->timeLoc, &e->time, SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->fftTextureLoc, fftTexture);

  float sampleRate = (float)AUDIO_SAMPLE_RATE;
  SetShaderValue(e->shader, e->sampleRateLoc, &sampleRate,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValueTexture(e->shader, e->gradientLUTLoc,
                        ColorLUTGetTexture(e->gradientLUT));
  SetShaderValue(e->shader, e->layersLoc, &cfg->layers, SHADER_UNIFORM_INT);
  SetShaderValue(e->shader, e->twistLoc, &cfg->twist, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->innerStretchLoc, &cfg->innerStretch,
                 SHADER_UNIFORM_FLOAT);
  // Invert: slider up = wider rings (min+max - value)
  float ringWidthInv = 52.0f - cfg->ringWidth;
  SetShaderValue(e->shader, e->ringWidthLoc, &ringWidthInv,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->diskThicknessLoc, &cfg->diskThickness,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->tiltLoc, &cfg->tilt, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->rotationLoc, &cfg->rotation,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->dustContrastLoc, &cfg->dustContrast,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starDensityLoc, &cfg->starDensity,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->starBrightLoc, &cfg->starBright,
                 SHADER_UNIFORM_FLOAT);
  // Invert: slider up = bigger bulge (min+max - value)
  float bulgeSizeInv = 55.0f - cfg->bulgeSize;
  SetShaderValue(e->shader, e->bulgeSizeLoc, &bulgeSizeInv,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->bulgeBrightLoc, &cfg->bulgeBright,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseFreqLoc, &cfg->baseFreq,
                 SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->maxFreqLoc, &cfg->maxFreq, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->gainLoc, &cfg->gain, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->curveLoc, &cfg->curve, SHADER_UNIFORM_FLOAT);
  SetShaderValue(e->shader, e->baseBrightLoc, &cfg->baseBright,
                 SHADER_UNIFORM_FLOAT);
}

void GalaxyEffectUninit(GalaxyEffect *e) {
  UnloadShader(e->shader);
  ColorLUTUninit(e->gradientLUT);
}

void GalaxyRegisterParams(GalaxyConfig *cfg) {
  ModEngineRegisterParam("galaxy.baseFreq", &cfg->baseFreq, 27.5f, 440.0f);
  ModEngineRegisterParam("galaxy.maxFreq", &cfg->maxFreq, 1000.0f, 16000.0f);
  ModEngineRegisterParam("galaxy.gain", &cfg->gain, 0.1f, 10.0f);
  ModEngineRegisterParam("galaxy.curve", &cfg->curve, 0.1f, 3.0f);
  ModEngineRegisterParam("galaxy.baseBright", &cfg->baseBright, 0.0f, 1.0f);
  ModEngineRegisterParam("galaxy.twist", &cfg->twist, 0.0f, 2.0f);
  ModEngineRegisterParam("galaxy.innerStretch", &cfg->innerStretch, 1.0f, 4.0f);
  ModEngineRegisterParam("galaxy.ringWidth", &cfg->ringWidth, 2.0f, 50.0f);
  ModEngineRegisterParam("galaxy.diskThickness", &cfg->diskThickness, 0.01f,
                         0.5f);
  ModEngineRegisterParam("galaxy.tilt", &cfg->tilt, 0.0f, 1.57f);
  ModEngineRegisterParam("galaxy.rotation", &cfg->rotation,
                         -ROTATION_OFFSET_MAX, ROTATION_OFFSET_MAX);
  ModEngineRegisterParam("galaxy.orbitSpeed", &cfg->orbitSpeed, -1.0f, 1.0f);
  ModEngineRegisterParam("galaxy.dustContrast", &cfg->dustContrast, 0.1f, 1.5f);
  ModEngineRegisterParam("galaxy.starDensity", &cfg->starDensity, 2.0f, 64.0f);
  ModEngineRegisterParam("galaxy.starBright", &cfg->starBright, 0.05f, 1.0f);
  ModEngineRegisterParam("galaxy.bulgeSize", &cfg->bulgeSize, 5.0f, 50.0f);
  ModEngineRegisterParam("galaxy.bulgeBright", &cfg->bulgeBright, 0.0f, 3.0f);
  ModEngineRegisterParam("galaxy.blendIntensity", &cfg->blendIntensity, 0.0f,
                         5.0f);
}

void SetupGalaxy(PostEffect *pe) {
  GalaxyEffectSetup(&pe->galaxy, &pe->effects.galaxy, pe->currentDeltaTime,
                    pe->fftTexture);
}

void SetupGalaxyBlend(PostEffect *pe) {
  BlendCompositorApply(pe->blendCompositor, pe->generatorScratch.texture,
                       pe->effects.galaxy.blendIntensity,
                       pe->effects.galaxy.blendMode);
}

// === UI ===

static void DrawGalaxyParams(EffectConfig *e, const ModSources *modSources,
                             ImU32 categoryGlow) {
  (void)categoryGlow;
  GalaxyConfig *g = &e->galaxy;

  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##galaxy", &g->baseFreq, "galaxy.baseFreq",
                    "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##galaxy", &g->maxFreq, "galaxy.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##galaxy", &g->gain, "galaxy.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##galaxy", &g->curve, "galaxy.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##galaxy", &g->baseBright, "galaxy.baseBright",
                    "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Rings##galaxy", &g->layers, 8, 25);
  ModulatableSlider("Twist##galaxy", &g->twist, "galaxy.twist", "%.2f",
                    modSources);
  ModulatableSlider("Inner Stretch##galaxy", &g->innerStretch,
                    "galaxy.innerStretch", "%.2f", modSources);
  ModulatableSlider("Ring Width##galaxy", &g->ringWidth, "galaxy.ringWidth",
                    "%.1f", modSources);
  ModulatableSlider("Disk Thickness##galaxy", &g->diskThickness,
                    "galaxy.diskThickness", "%.3f", modSources);

  // View
  ImGui::SeparatorText("View");
  ModulatableSlider("Tilt##galaxy", &g->tilt, "galaxy.tilt", "%.2f",
                    modSources);
  ModulatableSliderAngleDeg("Rotation##galaxy", &g->rotation, "galaxy.rotation",
                            modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSlider("Orbit Speed##galaxy", &g->orbitSpeed, "galaxy.orbitSpeed",
                    "%.2f", modSources);

  // Dust & Stars
  ImGui::SeparatorText("Dust & Stars");
  ModulatableSlider("Dust Contrast##galaxy", &g->dustContrast,
                    "galaxy.dustContrast", "%.2f", modSources);
  ModulatableSlider("Star Density##galaxy", &g->starDensity,
                    "galaxy.starDensity", "%.1f", modSources);
  ModulatableSlider("Star Bright##galaxy", &g->starBright, "galaxy.starBright",
                    "%.2f", modSources);

  // Bulge
  ImGui::SeparatorText("Bulge");
  ModulatableSlider("Bulge Size##galaxy", &g->bulgeSize, "galaxy.bulgeSize",
                    "%.1f", modSources);
  ModulatableSlider("Bulge Bright##galaxy", &g->bulgeBright,
                    "galaxy.bulgeBright", "%.2f", modSources);
}

// clang-format off
STANDARD_GENERATOR_OUTPUT(galaxy)
REGISTER_GENERATOR(TRANSFORM_GALAXY_BLEND, Galaxy, galaxy,
                   "Galaxy", SetupGalaxyBlend,
                   SetupGalaxy, 13, DrawGalaxyParams, DrawOutput_galaxy)
// clang-format on
