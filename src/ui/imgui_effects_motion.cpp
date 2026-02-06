#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

// Persistent section open states for transform categories
static bool sectionInfiniteZoom = false;
static bool sectionRadialStreak = false;
static bool sectionDrosteZoom = false;
static bool sectionDensityWaveSpiral = false;
static bool sectionShake = false;
static bool sectionRelativisticDoppler = false;

static void DrawMotionInfiniteZoom(EffectConfig *e,
                                   const ModSources *modSources,
                                   const ImU32 categoryGlow) {
  if (DrawSectionBegin("Infinite Zoom", categoryGlow, &sectionInfiniteZoom)) {
    const bool wasEnabled = e->infiniteZoom.enabled;
    ImGui::Checkbox("Enabled##infzoom", &e->infiniteZoom.enabled);
    if (!wasEnabled && e->infiniteZoom.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_INFINITE_ZOOM);
    }
    if (e->infiniteZoom.enabled) {
      ImGui::SliderFloat("Speed##infzoom", &e->infiniteZoom.speed, -2.0f, 2.0f,
                         "%.2f");
      ImGui::SliderFloat("Zoom Depth##infzoom", &e->infiniteZoom.zoomDepth,
                         1.0f, 5.0f, "%.1f");
      ImGui::SliderInt("Layers##infzoom", &e->infiniteZoom.layers, 2, 8);
      ModulatableSliderAngleDeg("Spiral Angle##infzoom",
                                &e->infiniteZoom.spiralAngle,
                                "infiniteZoom.spiralAngle", modSources);
      ModulatableSliderAngleDeg("Twist##infzoom", &e->infiniteZoom.spiralTwist,
                                "infiniteZoom.spiralTwist", modSources);
      ModulatableSliderAngleDeg("Layer Rotate##infzoom",
                                &e->infiniteZoom.layerRotate,
                                "infiniteZoom.layerRotate", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawMotionRadialBlur(EffectConfig *e, const ModSources *modSources,
                                 const ImU32 categoryGlow) {
  (void)modSources;
  if (DrawSectionBegin("Radial Blur", categoryGlow, &sectionRadialStreak)) {
    const bool wasEnabled = e->radialStreak.enabled;
    ImGui::Checkbox("Enabled##streak", &e->radialStreak.enabled);
    if (!wasEnabled && e->radialStreak.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_RADIAL_STREAK);
    }
    if (e->radialStreak.enabled) {
      ImGui::SliderInt("Samples##streak", &e->radialStreak.samples, 8, 32);
      ModulatableSlider("Streak Length##streak", &e->radialStreak.streakLength,
                        "radialStreak.streakLength", "%.2f", modSources);
      ModulatableSlider("Intensity##streak", &e->radialStreak.intensity,
                        "radialStreak.intensity", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

static void DrawMotionDroste(EffectConfig *e, const ModSources *modSources,
                             const ImU32 categoryGlow) {
  if (DrawSectionBegin("Droste Zoom", categoryGlow, &sectionDrosteZoom)) {
    const bool wasEnabled = e->drosteZoom.enabled;
    ImGui::Checkbox("Enabled##droste", &e->drosteZoom.enabled);
    if (!wasEnabled && e->drosteZoom.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_DROSTE_ZOOM);
    }
    if (e->drosteZoom.enabled) {
      ImGui::SliderFloat("Speed##droste", &e->drosteZoom.speed, -2.0f, 2.0f,
                         "%.2f");
      ModulatableSlider("Scale##droste", &e->drosteZoom.scale,
                        "drosteZoom.scale", "%.1f", modSources);
      ModulatableSliderAngleDeg("Spiral Angle##droste",
                                &e->drosteZoom.spiralAngle,
                                "drosteZoom.spiralAngle", modSources);
      ModulatableSlider("Shear##droste", &e->drosteZoom.shearCoeff,
                        "drosteZoom.shearCoeff", "%.2f", modSources);
      if (TreeNodeAccented("Masking##droste", categoryGlow)) {
        ModulatableSlider("Inner Radius##droste", &e->drosteZoom.innerRadius,
                          "drosteZoom.innerRadius", "%.2f", modSources);
        TreeNodeAccentedPop();
      }
      if (TreeNodeAccented("Spiral##droste", categoryGlow)) {
        ImGui::SliderInt("Branches##droste", &e->drosteZoom.branches, 1, 8);
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawMotionDensityWaveSpiral(EffectConfig *e,
                                        const ModSources *modSources,
                                        const ImU32 categoryGlow) {
  if (DrawSectionBegin("Density Wave Spiral", categoryGlow,
                       &sectionDensityWaveSpiral)) {
    const bool wasEnabled = e->densityWaveSpiral.enabled;
    ImGui::Checkbox("Enabled##dws", &e->densityWaveSpiral.enabled);
    if (!wasEnabled && e->densityWaveSpiral.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_DENSITY_WAVE_SPIRAL);
    }
    if (e->densityWaveSpiral.enabled) {
      DensityWaveSpiralConfig *dws = &e->densityWaveSpiral;
      if (TreeNodeAccented("Center##dws", categoryGlow)) {
        ImGui::SliderFloat("X##dwscenter", &dws->centerX, -0.5f, 0.5f, "%.2f");
        ImGui::SliderFloat("Y##dwscenter", &dws->centerY, -0.5f, 0.5f, "%.2f");
        TreeNodeAccentedPop();
      }
      if (TreeNodeAccented("Aspect##dws", categoryGlow)) {
        ImGui::SliderFloat("X##dwsaspect", &dws->aspectX, 0.1f, 1.0f, "%.2f");
        ImGui::SliderFloat("Y##dwsaspect", &dws->aspectY, 0.1f, 1.0f, "%.2f");
        TreeNodeAccentedPop();
      }
      ModulatableSliderAngleDeg("Tightness##dws", &dws->tightness,
                                "densityWaveSpiral.tightness", modSources);
      ModulatableSliderAngleDeg("Rotation Speed##dws", &dws->rotationSpeed,
                                "densityWaveSpiral.rotationSpeed", modSources,
                                "%.1f °/s");
      ModulatableSliderAngleDeg(
          "Global Rotation##dws", &dws->globalRotationSpeed,
          "densityWaveSpiral.globalRotationSpeed", modSources, "%.1f °/s");
      ModulatableSlider("Thickness##dws", &dws->thickness,
                        "densityWaveSpiral.thickness", "%.2f", modSources);
      ImGui::SliderInt("Ring Count##dws", &dws->ringCount, 10, 50);
      ImGui::SliderFloat("Falloff##dws", &dws->falloff, 0.5f, 2.0f, "%.2f");
    }
    DrawSectionEnd();
  }
}

static void DrawMotionShake(EffectConfig *e, const ModSources *modSources,
                            const ImU32 categoryGlow) {
  if (DrawSectionBegin("Shake", categoryGlow, &sectionShake)) {
    const bool wasEnabled = e->shake.enabled;
    ImGui::Checkbox("Enabled##shake", &e->shake.enabled);
    if (!wasEnabled && e->shake.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SHAKE);
    }
    if (e->shake.enabled) {
      ModulatableSlider("Intensity##shake", &e->shake.intensity,
                        "shake.intensity", "%.3f", modSources);
      ModulatableSliderInt("Samples##shake", &e->shake.samples, "shake.samples",
                           modSources);
      ModulatableSlider("Rate##shake", &e->shake.rate, "shake.rate", "%.1f Hz",
                        modSources);
      ImGui::Checkbox("Gaussian##shake", &e->shake.gaussian);
    }
    DrawSectionEnd();
  }
}

static void DrawMotionRelativisticDoppler(EffectConfig *e,
                                          const ModSources *modSources,
                                          const ImU32 categoryGlow) {
  if (DrawSectionBegin("Relativistic Doppler", categoryGlow,
                       &sectionRelativisticDoppler)) {
    const bool wasEnabled = e->relativisticDoppler.enabled;
    ImGui::Checkbox("Enabled##reldop", &e->relativisticDoppler.enabled);
    if (!wasEnabled && e->relativisticDoppler.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_RELATIVISTIC_DOPPLER);
    }
    if (e->relativisticDoppler.enabled) {
      ModulatableSlider("Velocity##reldop", &e->relativisticDoppler.velocity,
                        "relativisticDoppler.velocity", "%.2f", modSources);
      if (TreeNodeAccented("Center##reldop", categoryGlow)) {
        ModulatableSlider("X##reldopcenter", &e->relativisticDoppler.centerX,
                          "relativisticDoppler.centerX", "%.2f", modSources);
        ModulatableSlider("Y##reldopcenter", &e->relativisticDoppler.centerY,
                          "relativisticDoppler.centerY", "%.2f", modSources);
        TreeNodeAccentedPop();
      }
      ModulatableSlider("Aberration##reldop",
                        &e->relativisticDoppler.aberration,
                        "relativisticDoppler.aberration", "%.2f", modSources);
      ModulatableSlider("Color Shift##reldop",
                        &e->relativisticDoppler.colorShift,
                        "relativisticDoppler.colorShift", "%.2f", modSources);
      ModulatableSlider("Headlight##reldop", &e->relativisticDoppler.headlight,
                        "relativisticDoppler.headlight", "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

void DrawMotionCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(3);
  DrawCategoryHeader("Motion", categoryGlow);
  DrawMotionInfiniteZoom(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawMotionRadialBlur(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawMotionDroste(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawMotionDensityWaveSpiral(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawMotionShake(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawMotionRelativisticDoppler(e, modSources, categoryGlow);
}
