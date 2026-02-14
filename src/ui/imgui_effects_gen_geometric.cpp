#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/arc_strobe.h"
#include "effects/pitch_spiral.h"
#include "effects/signal_frames.h"
#include "effects/spectral_arcs.h"
#include "imgui.h"
#include "render/blend_mode.h"
#include "ui/imgui_effects_generators.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionSignalFrames = false;
static bool sectionArcStrobe = false;
static bool sectionPitchSpiral = false;
static bool sectionSpectralArcs = false;

static void DrawSignalFramesParams(SignalFramesConfig *cfg,
                                   const ModSources *modSources) {
  // Audio
  ImGui::SeparatorText("Audio");
  ImGui::SliderInt("Octaves##signalframes", &cfg->numOctaves, 1, 8);
  ModulatableSlider("Base Freq (Hz)##signalframes", &cfg->baseFreq,
                    "signalFrames.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##signalframes", &cfg->gain, "signalFrames.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##signalframes", &cfg->curve, "signalFrames.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##signalframes", &cfg->baseBright,
                    "signalFrames.baseBright", "%.2f", modSources);

  // Geometry
  ImGui::SeparatorText("Geometry");
  ModulatableSlider("Orbit Radius##signalframes", &cfg->orbitRadius,
                    "signalFrames.orbitRadius", "%.2f", modSources);
  ImGui::SliderFloat("Orbit Speed##signalframes", &cfg->orbitSpeed, 0.0f, 3.0f,
                     "%.2f");
  ModulatableSlider("Size Min##signalframes", &cfg->sizeMin,
                    "signalFrames.sizeMin", "%.2f", modSources);
  ModulatableSlider("Size Max##signalframes", &cfg->sizeMax,
                    "signalFrames.sizeMax", "%.2f", modSources);
  ModulatableSlider("Aspect Ratio##signalframes", &cfg->aspectRatio,
                    "signalFrames.aspectRatio", "%.2f", modSources);

  // Outline
  ImGui::SeparatorText("Outline");
  ModulatableSlider("Outline Thickness##signalframes", &cfg->outlineThickness,
                    "signalFrames.outlineThickness", "%.3f", modSources);
  ModulatableSlider("Glow Width##signalframes", &cfg->glowWidth,
                    "signalFrames.glowWidth", "%.3f", modSources);
  ModulatableSlider("Glow Intensity##signalframes", &cfg->glowIntensity,
                    "signalFrames.glowIntensity", "%.1f", modSources);

  // Sweep
  ImGui::SeparatorText("Sweep");
  ModulatableSlider("Sweep Speed##signalframes", &cfg->sweepSpeed,
                    "signalFrames.sweepSpeed", "%.2f", modSources);
  ModulatableSlider("Sweep Intensity##signalframes", &cfg->sweepIntensity,
                    "signalFrames.sweepIntensity", "%.3f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##signalframes", &cfg->rotationSpeed,
                            "signalFrames.rotationSpeed", modSources);
}

static void DrawSignalFramesOutput(SignalFramesConfig *cfg,
                                   const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##signalframes", &cfg->blendIntensity,
                    "signalFrames.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##signalframes", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsSignalFrames(EffectConfig *e,
                                       const ModSources *modSources,
                                       const ImU32 categoryGlow) {
  if (DrawSectionBegin("Signal Frames", categoryGlow, &sectionSignalFrames,
                       e->signalFrames.enabled)) {
    const bool wasEnabled = e->signalFrames.enabled;
    ImGui::Checkbox("Enabled##signalframes", &e->signalFrames.enabled);
    if (!wasEnabled && e->signalFrames.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SIGNAL_FRAMES_BLEND);
    }
    if (e->signalFrames.enabled) {
      DrawSignalFramesParams(&e->signalFrames, modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      DrawSignalFramesOutput(&e->signalFrames, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawArcStrobeParams(ArcStrobeConfig *cfg,
                                const ModSources *modSources) {
  // Audio
  ImGui::SeparatorText("Audio");
  ImGui::SliderInt("Octaves##arcstrobe", &cfg->numOctaves, 1, 8);
  ImGui::SliderInt("Segments/Octave##arcstrobe", &cfg->segmentsPerOctave, 4,
                   48);
  ModulatableSlider("Base Freq (Hz)##arcstrobe", &cfg->baseFreq,
                    "arcStrobe.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##arcstrobe", &cfg->gain, "arcStrobe.gain", "%.1f",
                    modSources);
  ModulatableSlider("Contrast##arcstrobe", &cfg->curve, "arcStrobe.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##arcstrobe", &cfg->baseBright,
                    "arcStrobe.baseBright", "%.2f", modSources);

  // Shape
  ImGui::SeparatorText("Shape");
  ModulatableSlider("Stride##arcstrobe", &cfg->orbitOffset,
                    "arcStrobe.orbitOffset", "%.2f", modSources);
  ModulatableSlider("Line Thickness##arcstrobe", &cfg->lineThickness,
                    "arcStrobe.lineThickness", "%.3f", modSources);

  // Lissajous
  ImGui::SeparatorText("Lissajous");
  DrawLissajousControls(&cfg->lissajous, "arcstrobe", "arcStrobe.lissajous",
                        modSources, 10.0f);

  // Glow
  ImGui::SeparatorText("Glow");
  ModulatableSlider("Glow Intensity##arcstrobe", &cfg->glowIntensity,
                    "arcStrobe.glowIntensity", "%.1f", modSources);

  // Strobe
  ImGui::SeparatorText("Strobe");
  ModulatableSlider("Strobe Speed##arcstrobe", &cfg->strobeSpeed,
                    "arcStrobe.strobeSpeed", "%.2f", modSources);
  ModulatableSlider("Strobe Decay##arcstrobe", &cfg->strobeDecay,
                    "arcStrobe.strobeDecay", "%.1f", modSources);
  ModulatableSlider("Strobe Boost##arcstrobe", &cfg->strobeBoost,
                    "arcStrobe.strobeBoost", "%.2f", modSources);
  ImGui::SliderInt("Strobe Stride##arcstrobe", &cfg->strobeStride, 1, 12);
}

static void DrawArcStrobeOutput(ArcStrobeConfig *cfg,
                                const ModSources *modSources) {
  ImGuiDrawColorMode(&cfg->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##arcstrobe", &cfg->blendIntensity,
                    "arcStrobe.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)cfg->blendMode;
  if (ImGui::Combo("Blend Mode##arcstrobe", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    cfg->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsArcStrobe(EffectConfig *e,
                                    const ModSources *modSources,
                                    const ImU32 categoryGlow) {
  if (DrawSectionBegin("Arc Strobe", categoryGlow, &sectionArcStrobe,
                       e->arcStrobe.enabled)) {
    const bool wasEnabled = e->arcStrobe.enabled;
    ImGui::Checkbox("Enabled##arcstrobe", &e->arcStrobe.enabled);
    if (!wasEnabled && e->arcStrobe.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_ARC_STROBE_BLEND);
    }
    if (e->arcStrobe.enabled) {
      DrawArcStrobeParams(&e->arcStrobe, modSources);
      DrawArcStrobeOutput(&e->arcStrobe, modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawGeneratorsPitchSpiral(EffectConfig *e,
                                      const ModSources *modSources,
                                      const ImU32 categoryGlow) {
  if (DrawSectionBegin("Pitch Spiral", categoryGlow, &sectionPitchSpiral,
                       e->pitchSpiral.enabled)) {
    const bool wasEnabled = e->pitchSpiral.enabled;
    ImGui::Checkbox("Enabled##pitchspiral", &e->pitchSpiral.enabled);
    if (!wasEnabled && e->pitchSpiral.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PITCH_SPIRAL_BLEND);
    }
    if (e->pitchSpiral.enabled) {
      PitchSpiralConfig *ps = &e->pitchSpiral;

      // Audio
      ImGui::SeparatorText("Audio");
      ImGui::SliderInt("Octaves##pitchspiral", &ps->numOctaves, 1, 8);
      ModulatableSlider("Base Freq (Hz)##pitchspiral", &ps->baseFreq,
                        "pitchSpiral.baseFreq", "%.1f", modSources);
      ModulatableSlider("Gain##pitchspiral", &ps->gain, "pitchSpiral.gain",
                        "%.1f", modSources);
      ModulatableSlider("Contrast##pitchspiral", &ps->curve,
                        "pitchSpiral.curve", "%.2f", modSources);
      ModulatableSlider("Base Bright##pitchspiral", &ps->baseBright,
                        "pitchSpiral.baseBright", "%.2f", modSources);

      // Geometry
      ImGui::SeparatorText("Geometry");
      ImGui::SliderInt("Turns##pitchspiral", &ps->numTurns, 4, 12);
      ModulatableSlider("Ring Spacing##pitchspiral", &ps->spiralSpacing,
                        "pitchSpiral.spiralSpacing", "%.3f", modSources);
      ModulatableSlider("Line Width##pitchspiral", &ps->lineWidth,
                        "pitchSpiral.lineWidth", "%.3f", modSources);
      ModulatableSlider("AA Softness##pitchspiral", &ps->blur,
                        "pitchSpiral.blur", "%.3f", modSources);

      // Tilt
      ImGui::SeparatorText("Tilt");
      ModulatableSlider("Tilt##pitchspiral", &ps->tilt, "pitchSpiral.tilt",
                        "%.2f", modSources);
      ModulatableSliderAngleDeg("Tilt Angle##pitchspiral", &ps->tiltAngle,
                                "pitchSpiral.tiltAngle", modSources);

      // Animation
      ImGui::SeparatorText("Animation");
      ModulatableSliderSpeedDeg("Rotation Speed##pitchspiral",
                                &ps->rotationSpeed, "pitchSpiral.rotationSpeed",
                                modSources);
      ModulatableSlider("Breath Speed##pitchspiral", &ps->breathSpeed,
                        "pitchSpiral.breathSpeed", "%.2f", modSources);
      ModulatableSlider("Breath Depth##pitchspiral", &ps->breathDepth,
                        "pitchSpiral.breathDepth", "%.3f", modSources);
      ModulatableSlider("Shape Exponent##pitchspiral", &ps->shapeExponent,
                        "pitchSpiral.shapeExponent", "%.2f", modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      // Color
      ImGuiDrawColorMode(&ps->gradient);

      // Output
      ImGui::SeparatorText("Output");
      ModulatableSlider("Blend Intensity##pitchspiral", &ps->blendIntensity,
                        "pitchSpiral.blendIntensity", "%.2f", modSources);
      int blendModeInt = (int)ps->blendMode;
      if (ImGui::Combo("Blend Mode##pitchspiral", &blendModeInt,
                       BLEND_MODE_NAMES, BLEND_MODE_NAME_COUNT)) {
        ps->blendMode = (EffectBlendMode)blendModeInt;
      }
    }
    DrawSectionEnd();
  }
}

static void DrawSpectralArcsParams(SpectralArcsConfig *sa,
                                   const ModSources *modSources) {
  // Audio
  ImGui::SeparatorText("Audio");
  ImGui::SliderInt("Octaves##spectralarcs", &sa->numOctaves, 1, 8);
  ModulatableSlider("Base Freq (Hz)##spectralarcs", &sa->baseFreq,
                    "spectralArcs.baseFreq", "%.1f", modSources);
  ModulatableSlider("Gain##spectralarcs", &sa->gain, "spectralArcs.gain",
                    "%.1f", modSources);
  ModulatableSlider("Contrast##spectralarcs", &sa->curve, "spectralArcs.curve",
                    "%.2f", modSources);
  ModulatableSlider("Base Bright##spectralarcs", &sa->baseBright,
                    "spectralArcs.baseBright", "%.2f", modSources);

  // Ring Layout
  ImGui::SeparatorText("Ring Layout");
  ModulatableSlider("Ring Scale##spectralarcs", &sa->ringScale,
                    "spectralArcs.ringScale", "%.2f", modSources);
  ModulatableSlider("Tilt##spectralarcs", &sa->tilt, "spectralArcs.tilt",
                    "%.2f", modSources);
  ModulatableSliderAngleDeg("Tilt Angle##spectralarcs", &sa->tiltAngle,
                            "spectralArcs.tiltAngle", modSources);

  // Arc Appearance
  ImGui::SeparatorText("Arcs");
  ModulatableSlider("Arc Width##spectralarcs", &sa->arcWidth,
                    "spectralArcs.arcWidth", "%.2f", modSources);
  ModulatableSlider("Glow Intensity##spectralarcs", &sa->glowIntensity,
                    "spectralArcs.glowIntensity", "%.3f", modSources);
  ModulatableSlider("Glow Falloff##spectralarcs", &sa->glowFalloff,
                    "spectralArcs.glowFalloff", "%.1f", modSources);

  // Animation
  ImGui::SeparatorText("Animation");
  ModulatableSliderSpeedDeg("Rotation Speed##spectralarcs", &sa->rotationSpeed,
                            "spectralArcs.rotationSpeed", modSources);
}

static void DrawSpectralArcsOutput(SpectralArcsConfig *sa,
                                   const ModSources *modSources) {
  ImGuiDrawColorMode(&sa->gradient);

  ImGui::SeparatorText("Output");
  ModulatableSlider("Blend Intensity##spectralarcs", &sa->blendIntensity,
                    "spectralArcs.blendIntensity", "%.2f", modSources);
  int blendModeInt = (int)sa->blendMode;
  if (ImGui::Combo("Blend Mode##spectralarcs", &blendModeInt, BLEND_MODE_NAMES,
                   BLEND_MODE_NAME_COUNT)) {
    sa->blendMode = (EffectBlendMode)blendModeInt;
  }
}

static void DrawGeneratorsSpectralArcs(EffectConfig *e,
                                       const ModSources *modSources,
                                       const ImU32 categoryGlow) {
  if (DrawSectionBegin("Spectral Arcs", categoryGlow, &sectionSpectralArcs,
                       e->spectralArcs.enabled)) {
    const bool wasEnabled = e->spectralArcs.enabled;
    ImGui::Checkbox("Enabled##spectralarcs", &e->spectralArcs.enabled);
    if (!wasEnabled && e->spectralArcs.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SPECTRAL_ARCS_BLEND);
    }
    if (e->spectralArcs.enabled) {
      SpectralArcsConfig *sa = &e->spectralArcs;
      DrawSpectralArcsParams(sa, modSources);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      DrawSpectralArcsOutput(sa, modSources);
    }
    DrawSectionEnd();
  }
}

void DrawGeneratorsGeometric(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(0);
  DrawCategoryHeader("Geometric", categoryGlow);
  DrawGeneratorsSignalFrames(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsArcStrobe(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsPitchSpiral(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawGeneratorsSpectralArcs(e, modSources, categoryGlow);
}
