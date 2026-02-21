#include "automation/mod_sources.h"
#include "config/attractor_types.h"
#include "config/effect_config.h"
#include "effects/attractor_lines.h"
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

static bool sectionAttractorLines = false;
static bool sectionConstellation = false;
static bool sectionFilaments = false;
static bool sectionMuons = false;
static bool sectionSlashes = false;

static void DrawGeneratorsConstellation(EffectConfig *e,
                                        const ModSources *modSources,
                                        const ImU32 categoryGlow) {
  if (DrawSectionBegin("Constellation", categoryGlow, &sectionConstellation,
                       e->constellation.enabled)) {
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
      ImGui::SliderFloat("Wave Freq##constellation", &c->waveFreq, 0.1f, 2.0f,
                         "%.2f");
      ModulatableSlider("Wave Amp##constellation", &c->waveAmp,
                        "constellation.waveAmp", "%.2f", modSources);
      ModulatableSlider("Wave Speed##constellation", &c->waveSpeed,
                        "constellation.waveSpeed", "%.2f", modSources);
      ImGui::SliderFloat("Wave Center X##constellation", &c->waveCenterX, -2.0f,
                         3.0f, "%.2f");
      ImGui::SliderFloat("Wave Center Y##constellation", &c->waveCenterY, -2.0f,
                         3.0f, "%.2f");

      ModulatableSlider("Wave Influence##constellation", &c->waveInfluence,
                        "constellation.waveInfluence", "%.2f", modSources);

      // Depth
      ImGui::SliderInt("Depth Layers##constellation", &c->depthLayers, 1, 3);

      // Point rendering
      ImGui::SeparatorText("Points");
      ModulatableSlider("Point Size##constellation", &c->pointSize,
                        "constellation.pointSize", "%.2f", modSources);
      ModulatableSlider("Point Bright##constellation", &c->pointBrightness,
                        "constellation.pointBrightness", "%.2f", modSources);
      ModulatableSlider("Point Opacity##constellation", &c->pointOpacity,
                        "constellation.pointOpacity", "%.2f", modSources);
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
  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##filaments", &cfg->baseFreq,
                    "filaments.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##filaments", &cfg->maxFreq,
                    "filaments.maxFreq", "%.0f", modSources);
  ModulatableSlider("Gain##filaments", &cfg->gain, "filaments.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##filaments", &cfg->curve, "filaments.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##filaments", &cfg->baseBright,
                    "filaments.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Filaments##filaments", &cfg->filaments, 4, 256);
  ModulatableSlider("Radius##filaments", &cfg->radius, "filaments.radius",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Spread##filaments", &cfg->spread,
                            "filaments.spread", modSources);
  ModulatableSliderAngleDeg("Step Angle##filaments", &cfg->stepAngle,
                            "filaments.stepAngle", modSources);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##filaments", &cfg->glowIntensity,
                    "filaments.glowIntensity", "%.1f", modSources);

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
  if (DrawSectionBegin("Filaments", categoryGlow, &sectionFilaments,
                       e->filaments.enabled)) {
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
  // Audio
  ImGui::SeparatorText("Audio");
  ModulatableSlider("Base Freq (Hz)##slashes", &cfg->baseFreq,
                    "slashes.baseFreq", "%.1f", modSources);
  ModulatableSlider("Max Freq (Hz)##slashes", &cfg->maxFreq, "slashes.maxFreq",
                    "%.0f", modSources);
  ModulatableSlider("Gain##slashes", &cfg->gain, "slashes.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##slashes", &cfg->curve, "slashes.curve", "%.2f",
                    modSources);
  ModulatableSlider("Base Bright##slashes", &cfg->baseBright,
                    "slashes.baseBright", "%.2f", modSources);

  // Timing
  ImGui::SeparatorText("Timing");
  ModulatableSlider("Tick Rate##slashes", &cfg->tickRate, "slashes.tickRate",
                    "%.1f", modSources);
  ModulatableSlider("Envelope Sharp##slashes", &cfg->envelopeSharp,
                    "slashes.envelopeSharp", "%.1f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ImGui::SliderInt("Bars##slashes", &cfg->bars, 4, 256);
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
  if (DrawSectionBegin("Slashes", categoryGlow, &sectionSlashes,
                       e->slashes.enabled)) {
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
  if (DrawSectionBegin("Muons", categoryGlow, &sectionMuons,
                       e->muons.enabled)) {
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
      ImGui::SliderInt("Octaves##muons", &m->turbulenceOctaves, 2, 12);
      ModulatableSlider("Turbulence##muons", &m->turbulenceStrength,
                        "muons.turbulenceStrength", "%.2f", modSources);
      ModulatableSlider("Ring Thickness##muons", &m->ringThickness,
                        "muons.ringThickness", "%.3f", modSources);
      ModulatableSlider("Camera Distance##muons", &m->cameraDistance,
                        "muons.cameraDistance", "%.1f", modSources);

      // Trails
      ImGui::SeparatorText("Trails");
      ModulatableSlider("Decay Half-Life##muons", &m->decayHalfLife,
                        "muons.decayHalfLife", "%.1f", modSources);

      // Audio
      ImGui::SeparatorText("Audio");
      ModulatableSlider("Base Freq (Hz)##muons", &m->baseFreq, "muons.baseFreq",
                        "%.1f", modSources);
      ModulatableSlider("Max Freq (Hz)##muons", &m->maxFreq, "muons.maxFreq",
                        "%.0f", modSources);
      ModulatableSlider("Gain##muons", &m->gain, "muons.gain", "%.1f",
                        modSources);
      ModulatableSlider("Contrast##muons", &m->curve, "muons.curve", "%.2f",
                        modSources);
      ModulatableSlider("Base Bright##muons", &m->baseBright,
                        "muons.baseBright", "%.2f", modSources);

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

static void DrawAttractorSystemParams(AttractorLinesConfig *c,
                                      const ModSources * /*modSources*/) {
  if (c->attractorType == ATTRACTOR_LORENZ) {
    ImGui::SliderFloat("Sigma##attractorLines", &c->sigma, 1.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Rho##attractorLines", &c->rho, 10.0f, 50.0f, "%.1f");
    ImGui::SliderFloat("Beta##attractorLines", &c->beta, 0.5f, 5.0f, "%.2f");
  } else if (c->attractorType == ATTRACTOR_ROSSLER) {
    ImGui::SliderFloat("Rossler C##attractorLines", &c->rosslerC, 2.0f, 12.0f,
                       "%.2f");
  } else if (c->attractorType == ATTRACTOR_THOMAS) {
    ImGui::SliderFloat("Thomas B##attractorLines", &c->thomasB, 0.1f, 0.3f,
                       "%.3f");
  } else if (c->attractorType == ATTRACTOR_DADRAS) {
    ImGui::SliderFloat("Dadras A##attractorLines", &c->dadrasA, 1.0f, 5.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras B##attractorLines", &c->dadrasB, 1.0f, 5.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras C##attractorLines", &c->dadrasC, 0.5f, 3.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras D##attractorLines", &c->dadrasD, 0.5f, 4.0f,
                       "%.1f");
    ImGui::SliderFloat("Dadras E##attractorLines", &c->dadrasE, 4.0f, 15.0f,
                       "%.1f");
  } else if (c->attractorType == ATTRACTOR_CHUA) {
    ImGui::SliderFloat("Alpha##chua", &c->chuaAlpha, 5.0f, 30.0f, "%.1f");
    ImGui::SliderFloat("Gamma##chua", &c->chuaGamma, 10.0f, 40.0f, "%.2f");
    ImGui::SliderFloat("M0##chua", &c->chuaM0, -3.0f, 0.0f, "%.1f");
    ImGui::SliderFloat("M1##chua", &c->chuaM1, -1.0f, 1.0f, "%.2f");
  }
}

static void DrawAttractorTransformOutput(AttractorLinesConfig *c,
                                         const ModSources *modSources) {
  ImGui::SeparatorText("Transform");
  ModulatableSlider("X Position##attractorLines", &c->x, "attractorLines.x",
                    "%.2f", modSources);
  ModulatableSlider("Y Position##attractorLines", &c->y, "attractorLines.y",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Angle X##attractorLines", &c->rotationAngleX,
                            "attractorLines.rotationAngleX", modSources);
  ModulatableSliderAngleDeg("Angle Y##attractorLines", &c->rotationAngleY,
                            "attractorLines.rotationAngleY", modSources);
  ModulatableSliderAngleDeg("Angle Z##attractorLines", &c->rotationAngleZ,
                            "attractorLines.rotationAngleZ", modSources);
  ModulatableSliderSpeedDeg("Spin X##attractorLines", &c->rotationSpeedX,
                            "attractorLines.rotationSpeedX", modSources);
  ModulatableSliderSpeedDeg("Spin Y##attractorLines", &c->rotationSpeedY,
                            "attractorLines.rotationSpeedY", modSources);
  ModulatableSliderSpeedDeg("Spin Z##attractorLines", &c->rotationSpeedZ,
                            "attractorLines.rotationSpeedZ", modSources);

  ImGui::SeparatorText("Output");
  ImGuiDrawColorMode(&c->gradient);
  int blendModeInt = (int)c->blendMode;
  if (ImGui::Combo("Blend Mode##attractorLines", &blendModeInt,
                   BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
    c->blendMode = (EffectBlendMode)blendModeInt;
  }
  ModulatableSlider("Blend Intensity##attractorLines", &c->blendIntensity,
                    "attractorLines.blendIntensity", "%.2f", modSources);
}

static void DrawGeneratorsAttractorLines(EffectConfig *e,
                                         const ModSources *modSources,
                                         const ImU32 categoryGlow) {
  if (DrawSectionBegin("Attractor Lines", categoryGlow, &sectionAttractorLines,
                       e->attractorLines.enabled)) {
    const bool wasEnabled = e->attractorLines.enabled;
    ImGui::Checkbox("Enabled##attractorLines", &e->attractorLines.enabled);
    if (!wasEnabled && e->attractorLines.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_ATTRACTOR_LINES_BLEND);
    }
    if (e->attractorLines.enabled) {
      AttractorLinesConfig *c = &e->attractorLines;

      const char *attractorNames[] = {"Lorenz", "Rossler", "Aizawa",
                                      "Thomas", "Dadras",  "Chua"};
      int attractorType = (int)c->attractorType;
      if (ImGui::Combo("Attractor Type##attractorLines", &attractorType,
                       attractorNames, ATTRACTOR_COUNT)) {
        c->attractorType = (AttractorType)attractorType;
      }

      DrawAttractorSystemParams(c, modSources);

      ImGui::SeparatorText("Tracing");
      ImGui::SliderInt("Particles##attractorLines", &c->numParticles, 1, 16);
      ImGui::SliderInt("Steps##attractorLines", &c->steps, 4, 48);
      ModulatableSlider("Speed##attractorLines", &c->speed,
                        "attractorLines.speed", "%.2f", modSources);
      ModulatableSlider("View Scale##attractorLines", &c->viewScale,
                        "attractorLines.viewScale", "%.3f", modSources);

      ImGui::SeparatorText("Appearance");
      ModulatableSlider("Intensity##attractorLines", &c->intensity,
                        "attractorLines.intensity", "%.2f", modSources);
      ModulatableSlider("Decay Half-Life##attractorLines", &c->decayHalfLife,
                        "attractorLines.decayHalfLife", "%.1f", modSources);
      ModulatableSlider("Focus##attractorLines", &c->focus,
                        "attractorLines.focus", "%.1f", modSources);
      ModulatableSlider("Max Speed##attractorLines", &c->maxSpeed,
                        "attractorLines.maxSpeed", "%.0f", modSources);

      DrawAttractorTransformOutput(c, modSources);
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
  ImGui::Spacing();
  DrawGeneratorsAttractorLines(e, modSources, categoryGlow);
}
