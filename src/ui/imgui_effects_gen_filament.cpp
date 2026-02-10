#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/constellation.h"
#include "effects/filaments.h"
#include "effects/muons.h"
#include "effects/slashes.h"
#include "imgui.h"
#include "render/blend_mode.h"
#include "ui/imgui_effects_generators.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionConstellation = false;
static bool sectionFilaments = false;
static bool sectionMuons = false;
static bool sectionSlashes = false;

static void DrawGeneratorsConstellation(EffectConfig *e,
                                        const ModSources *modSources,
                                        const ImU32 categoryGlow) {
  if (DrawSectionBegin("Constellation", categoryGlow, &sectionConstellation)) {
    const bool wasEnabled = e->constellation.enabled;
    ImGui::Checkbox("Enabled##constellation", &e->constellation.enabled);
    if (!wasEnabled && e->constellation.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CONSTELLATION_BLEND);
    }
    if (e->constellation.enabled) {
      ConstellationConfig *c = &e->constellation;

      // Grid and animation
      ModulatableSlider("Grid Scale##constellation", &c->gridScale,
                        "constellation.gridScale", "%.1f", modSources);
      ModulatableSlider("Anim Speed##constellation", &c->animSpeed,
                        "constellation.animSpeed", "%.2f", modSources);
      ModulatableSlider("Wander##constellation", &c->wanderAmp,
                        "constellation.wanderAmp", "%.2f", modSources);

      // Wave overlay
      ImGui::SeparatorText("Wave");
      ImGui::SliderFloat("Wave Freq##constellation", &c->waveFreq, 0.1f, 5.0f,
                         "%.2f");
      ModulatableSlider("Wave Amp##constellation", &c->waveAmp,
                        "constellation.waveAmp", "%.2f", modSources);
      ModulatableSlider("Wave Speed##constellation", &c->waveSpeed,
                        "constellation.waveSpeed", "%.2f", modSources);
      ImGui::SliderFloat("Wave Center X##constellation", &c->waveCenterX, -2.0f,
                         3.0f, "%.2f");
      ImGui::SliderFloat("Wave Center Y##constellation", &c->waveCenterY, -2.0f,
                         3.0f, "%.2f");

      // Point rendering
      ImGui::SeparatorText("Points");
      ModulatableSlider("Point Size##constellation", &c->pointSize,
                        "constellation.pointSize", "%.2f", modSources);
      ModulatableSlider("Point Bright##constellation", &c->pointBrightness,
                        "constellation.pointBrightness", "%.2f", modSources);
      ImGuiDrawColorMode(&c->pointGradient);

      // Line rendering
      ImGui::SeparatorText("Lines");
      ImGui::SliderFloat("Line Width##constellation", &c->lineThickness, 0.01f,
                         0.1f, "%.3f");
      ModulatableSlider("Max Line Len##constellation", &c->maxLineLen,
                        "constellation.maxLineLen", "%.2f", modSources);
      ModulatableSlider("Line Opacity##constellation", &c->lineOpacity,
                        "constellation.lineOpacity", "%.2f", modSources);
      ImGui::Checkbox("Interpolate Line Color##constellation",
                      &c->interpolateLineColor);
      ImGuiDrawColorMode(&c->lineGradient);

      // Triangle fill
      ImGui::SeparatorText("Triangles");
      ImGui::Checkbox("Fill Triangles##constellation", &c->fillEnabled);
      if (c->fillEnabled) {
        ModulatableSlider("Fill Opacity##constellation", &c->fillOpacity,
                          "constellation.fillOpacity", "%.2f", modSources);
        ImGui::SliderFloat("Fill Threshold##constellation", &c->fillThreshold,
                           1.0f, 4.0f, "%.1f");
      }

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##constellation", &c->blendIntensity,
                        "constellation.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)c->blendMode;
      if (ImGui::Combo("Blend Mode##constellation", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        c->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawFilamentsParams(FilamentsConfig *cfg,
                                const ModSources *modSources) {
  // FFT
  ImGui::SeparatorText("FFT");
  ImGui::SliderInt("Octaves##filaments", &cfg->numOctaves, 1, 8);
  ModulatableSlider("Base Freq (Hz)##filaments", &cfg->baseFreq,
                    "filaments.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##filaments", &cfg->gain, "filaments.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##filaments", &cfg->curve, "filaments.curve",
                    "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Radius##filaments", &cfg->radius, "filaments.radius",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Spread##filaments", &cfg->spread,
                            "filaments.spread", modSources);
  ModulatableSliderAngleDeg("Step Angle##filaments", &cfg->stepAngle,
                            "filaments.stepAngle", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##filaments", &cfg->glowIntensity,
                    "filaments.glowIntensity", "%.3f", modSources);
  ModulatableSlider("Falloff##filaments", &cfg->falloffExponent,
                    "filaments.falloffExponent", "%.2f", modSources);
  ModulatableSlider("Base Bright##filaments", &cfg->baseBright,
                    "filaments.baseBright", "%.2f", modSources);

  // Noise
  ImGui::SeparatorText("Noise");
  ModulatableSlider("Noise Strength##filaments", &cfg->noiseStrength,
                    "filaments.noiseStrength", "%.2f", modSources);
  ModulatableSlider("Noise Speed##filaments", &cfg->noiseSpeed,
                    "filaments.noiseSpeed", "%.1f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##filaments", &cfg->rotationSpeed,
                            "filaments.rotationSpeed", modSources);
}

static void DrawFilamentsOutput(FilamentsConfig *cfg,
                                const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##filaments", &cfg->blendIntensity,
                    "filaments.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##filaments", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsFilaments(EffectConfig *e,
                                    const ModSources *modSources,
                                    const ImU32 categoryGlow) {
  if (DrawSectionBegin("Filaments", categoryGlow, &sectionFilaments)) {
    const bool wasEnabled = e->filaments.enabled;
    ImGui::Checkbox("Enabled##filaments", &e->filaments.enabled);
    if (!wasEnabled && e->filaments.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_FILAMENTS_BLEND);
    }
    if (e->filaments.enabled) {
      DrawFilamentsParams(&e->filaments, modSources);
      DrawFilamentsOutput(&e->filaments, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawSlashesParams(SlashesConfig *cfg,
                              const ModSources *modSources) {
  // FFT
  ImGui::SeparatorText("FFT");
  ImGui::SliderInt("Octaves##slashes", &cfg->numOctaves, 1, 8);
  ModulatableSlider("Base Freq (Hz)##slashes", &cfg->baseFreq,
                    "slashes.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##slashes", &cfg->gain, "slashes.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##slashes", &cfg->curve, "slashes.curve", "%.2f",
                    modSources);

  // Timing
  ImGui::SeparatorText("Timing");
  ModulatableSlider("Tick Rate##slashes", &cfg->tickRate, "slashes.tickRate",
                    "%.1f", modSources);
  ModulatableSlider("Envelope Sharp##slashes", &cfg->envelopeSharp,
                    "slashes.envelopeSharp", "%.1f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Bar Length##slashes", &cfg->maxBarLength,
                    "slashes.maxBarLength", "%.2f", modSources);
  ModulatableSlider("Bar Thickness##slashes", &cfg->barThickness,
                    "slashes.barThickness", "%.3f", modSources);
  ModulatableSlider("Thickness Var##slashes", &cfg->thicknessVariation,
                    "slashes.thicknessVariation", "%.2f", modSources);
  ModulatableSlider("Scatter##slashes", &cfg->scatter, "slashes.scatter",
                    "%.2f", modSources);
  ModulatableSlider("Rotation Depth##slashes", &cfg->rotationDepth,
                    "slashes.rotationDepth", "%.2f", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Softness##slashes", &cfg->glowSoftness,
                    "slashes.glowSoftness", "%.3f", modSources);
  ModulatableSlider("Base Bright##slashes", &cfg->baseBright,
                    "slashes.baseBright", "%.2f", modSources);
}

static void DrawSlashesOutput(SlashesConfig *cfg,
                              const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##slashes", &cfg->blendIntensity,
                    "slashes.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##slashes", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsSlashes(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("Slashes", categoryGlow, &sectionSlashes)) {
    const bool wasEnabled = e->slashes.enabled;
    ImGui::Checkbox("Enabled##slashes", &e->slashes.enabled);
    if (!wasEnabled && e->slashes.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SLASHES_BLEND);
    }
    if (e->slashes.enabled) {
      DrawSlashesParams(&e->slashes, modSources);
      DrawSlashesOutput(&e->slashes, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsMuons(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Muons", categoryGlow, &sectionMuons)) {
    const bool wasEnabled = e->muons.enabled;
    ImGui::Checkbox("Enabled##muons", &e->muons.enabled);
    if (!wasEnabled && e->muons.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MUONS_BLEND);
    }
    if (e->muons.enabled) {
      MuonsConfig *m = &e->muons;

      // Raymarching
      ImGui::SeparatorText("Raymarching");
      ImGui::SliderInt("March Steps##muons", &m->marchSteps, 4, 40);
      ImGui::SliderInt("Octaves##muons", &m->turbulenceOctaves, 1, 12);
      ModulatableSlider("Turbulence##muons", &m->turbulenceStrength,
                        "muons.turbulenceStrength", "%.2f", modSources);
      ModulatableSlider("Ring Thickness##muons", &m->ringThickness,
                        "muons.ringThickness", "%.3f", modSources);
      ModulatableSlider("Camera Distance##muons", &m->cameraDistance,
                        "muons.cameraDistance", "%.1f", modSources);

      // Color
      ImGui::SeparatorText("Color");
      ModulatableSlider("Color Freq##muons", &m->colorFreq, "muons.colorFreq",
                        "%.1f", modSources);
      ModulatableSlider("Color Speed##muons", &m->colorSpeed,
                        "muons.colorSpeed", "%.2f", modSources);
      ImGuiDrawColorMode(&m->gradient);

      // Tonemap
      ImGui::SeparatorText("Tonemap");
      ModulatableSlider("Brightness##muons", &m->brightness, "muons.brightness",
                        "%.2f", modSources);
      ModulatableSlider("Exposure##muons", &m->exposure, "muons.exposure",
                        "%.0f", modSources);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##muons", &m->blendIntensity,
                        "muons.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)m->blendMode;
      if (ImGui::Combo("Blend Mode##muons", &blendModeInt, BLEND_MODE_NAMES,
                       BLEND_MODE_NAME_COUNT)) {
        m->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

void DrawGeneratorsFilament(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(1);
  DrawCategoryHeader("Filament", categoryGlow);
  DrawGeneratorsConstellation(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsFilaments(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsSlashes(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsMuons(e, modSources, categoryGlow);
}
