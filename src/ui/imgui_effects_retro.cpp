#include "automation/mod_sources.h"
#include "config/effect_config.h"
#include "effects/lattice_crush.h"
#include "imgui.h"
#include "ui/imgui_effects_transforms.h"
#include "ui/imgui_panels.h"
#include "ui/modulatable_slider.h"
#include "ui/theme.h"
#include "ui/ui_units.h"

static bool sectionPixelation = false;
static bool sectionGlitch = false;
static bool sectionCrt = false;
static bool sectionAsciiArt = false;
static bool sectionMatrixRain = false;
static bool sectionSynthwave = false;
static bool sectionLatticeCrush = false;

static const char *WALK_MODE_NAMES[] = {"Original",        "Rotating Dir",
                                        "Offset Neighbor", "Alternating Snap",
                                        "Cross-Coupled",   "Asymmetric Hash"};
static const int WALK_MODE_COUNT = 6;

static void DrawRetroPixelation(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Pixelation", categoryGlow, &sectionPixelation,
                       e->pixelation.enabled)) {
    const bool wasEnabled = e->pixelation.enabled;
    ImGui::Checkbox("Enabled##pixel", &e->pixelation.enabled);
    if (!wasEnabled && e->pixelation.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_PIXELATION);
    }
    if (e->pixelation.enabled) {
      ModulatableSlider("Cell Count##pixel", &e->pixelation.cellCount,
                        "pixelation.cellCount", "%.0f", modSources);
      ImGui::SliderInt("Posterize##pixel", &e->pixelation.posterizeLevels, 0,
                       16);
      if (e->pixelation.posterizeLevels > 0) {
        ModulatableSliderInt("Dither Scale##pixel", &e->pixelation.ditherScale,
                             "pixelation.ditherScale", modSources);
      }
    }
    DrawSectionEnd();
  }
}

static void DrawRetroGlitch(EffectConfig *e, const ModSources *modSources,
                            const ImU32 categoryGlow) {
  if (DrawSectionBegin("Glitch", categoryGlow, &sectionGlitch,
                       e->glitch.enabled)) {
    const bool wasEnabled = e->glitch.enabled;
    ImGui::Checkbox("Enabled##glitch", &e->glitch.enabled);
    if (!wasEnabled && e->glitch.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_GLITCH);
    }
    if (e->glitch.enabled) {
      GlitchConfig *g = &e->glitch;

      if (TreeNodeAccented("Analog##glitch", categoryGlow)) {
        ModulatableSlider("Intensity##analog", &g->analogIntensity,
                          "glitch.analogIntensity", "%.3f", modSources);
        ModulatableSlider("Aberration##analog", &g->aberration,
                          "glitch.aberration", "%.1f px", modSources);
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Digital##glitch", categoryGlow)) {
        ModulatableSlider("Block Threshold##digital", &g->blockThreshold,
                          "glitch.blockThreshold", "%.2f", modSources);
        ModulatableSlider("Block Offset##digital", &g->blockOffset,
                          "glitch.blockOffset", "%.2f", modSources);
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("VHS##glitch", categoryGlow)) {
        ImGui::Checkbox("Enabled##vhs", &g->vhsEnabled);
        if (g->vhsEnabled) {
          ImGui::SliderFloat("Tracking Bars##vhs", &g->trackingBarIntensity,
                             0.0f, 0.05f, "%.3f");
          ImGui::SliderFloat("Scanline Noise##vhs", &g->scanlineNoiseIntensity,
                             0.0f, 0.02f, "%.4f");
          ImGui::SliderFloat("Color Drift##vhs", &g->colorDriftIntensity, 0.0f,
                             2.0f, "%.2f");
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Datamosh##glitch", categoryGlow)) {
        ImGui::Checkbox("Enabled##datamosh", &g->datamoshEnabled);
        if (g->datamoshEnabled) {
          ModulatableSlider("Intensity##datamosh", &g->datamoshIntensity,
                            "glitch.datamoshIntensity", "%.2f", modSources);
          ModulatableSlider("Min Res##datamosh", &g->datamoshMin,
                            "glitch.datamoshMin", "%.0f", modSources);
          ModulatableSlider("Max Res##datamosh", &g->datamoshMax,
                            "glitch.datamoshMax", "%.0f", modSources);
          ImGui::SliderFloat("Speed##datamosh", &g->datamoshSpeed, 1.0f, 30.0f,
                             "%.1f");
          ImGui::SliderFloat("Bands##datamosh", &g->datamoshBands, 1.0f, 32.0f,
                             "%.0f");
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Slice##glitch", categoryGlow)) {
        ImGui::Text("Row (Horizontal)");
        ImGui::Checkbox("Enabled##rowslice", &g->rowSliceEnabled);
        if (g->rowSliceEnabled) {
          ModulatableSlider("Intensity##rowslice", &g->rowSliceIntensity,
                            "glitch.rowSliceIntensity", "%.3f", modSources);
          ImGui::SliderFloat("Burst Freq##rowslice", &g->rowSliceBurstFreq,
                             0.5f, 20.0f, "%.1f Hz");
          ImGui::SliderFloat("Burst Power##rowslice", &g->rowSliceBurstPower,
                             1.0f, 15.0f, "%.1f");
          ImGui::SliderFloat("Columns##rowslice", &g->rowSliceColumns, 8.0f,
                             128.0f, "%.0f");
        }
        ImGui::Spacing();
        ImGui::Text("Column (Vertical)");
        ImGui::Checkbox("Enabled##colslice", &g->colSliceEnabled);
        if (g->colSliceEnabled) {
          ModulatableSlider("Intensity##colslice", &g->colSliceIntensity,
                            "glitch.colSliceIntensity", "%.3f", modSources);
          ImGui::SliderFloat("Burst Freq##colslice", &g->colSliceBurstFreq,
                             0.5f, 20.0f, "%.1f Hz");
          ImGui::SliderFloat("Burst Power##colslice", &g->colSliceBurstPower,
                             1.0f, 15.0f, "%.1f");
          ImGui::SliderFloat("Rows##colslice", &g->colSliceRows, 8.0f, 128.0f,
                             "%.0f");
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Diagonal Bands##glitch", categoryGlow)) {
        ImGui::Checkbox("Enabled##diagbands", &g->diagonalBandsEnabled);
        if (g->diagonalBandsEnabled) {
          ImGui::SliderFloat("Band Count##diagbands", &g->diagonalBandCount,
                             2.0f, 32.0f, "%.0f");
          ModulatableSlider("Displace##diagbands", &g->diagonalBandDisplace,
                            "glitch.diagonalBandDisplace", "%.3f", modSources);
          ImGui::SliderFloat("Speed##diagbands", &g->diagonalBandSpeed, 0.0f,
                             10.0f, "%.1f");
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Block Mask##glitch", categoryGlow)) {
        ImGui::Checkbox("Enabled##blockmask", &g->blockMaskEnabled);
        if (g->blockMaskEnabled) {
          ModulatableSlider("Intensity##blockmask", &g->blockMaskIntensity,
                            "glitch.blockMaskIntensity", "%.2f", modSources);
          ImGui::SliderInt("Min Size##blockmask", &g->blockMaskMinSize, 1, 10);
          ImGui::SliderInt("Max Size##blockmask", &g->blockMaskMaxSize, 5, 20);
          float tint[3] = {g->blockMaskTintR, g->blockMaskTintG,
                           g->blockMaskTintB};
          if (ImGui::ColorEdit3("Tint##blockmask", tint)) {
            g->blockMaskTintR = tint[0];
            g->blockMaskTintG = tint[1];
            g->blockMaskTintB = tint[2];
          }
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Temporal##glitch", categoryGlow)) {
        ImGui::Checkbox("Enabled##temporal", &g->temporalJitterEnabled);
        if (g->temporalJitterEnabled) {
          ModulatableSlider("Amount##temporal", &g->temporalJitterAmount,
                            "glitch.temporalJitterAmount", "%.3f", modSources);
          ModulatableSlider("Gate##temporal", &g->temporalJitterGate,
                            "glitch.temporalJitterGate", "%.2f", modSources);
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Block Multiply##glitch", categoryGlow)) {
        ImGui::Checkbox("Enabled##blockmultiply", &g->blockMultiplyEnabled);
        if (g->blockMultiplyEnabled) {
          ModulatableSlider("Block Size##blockmultiply", &g->blockMultiplySize,
                            "glitch.blockMultiplySize", "%.1f", modSources);
          ModulatableSlider("Distortion##blockmultiply",
                            &g->blockMultiplyControl,
                            "glitch.blockMultiplyControl", "%.3f", modSources);
          ImGui::SliderInt("Iterations##blockmultiply",
                           &g->blockMultiplyIterations, 1, 8);
          ModulatableSlider(
              "Intensity##blockmultiply", &g->blockMultiplyIntensity,
              "glitch.blockMultiplyIntensity", "%.2f", modSources);
        }
        TreeNodeAccentedPop();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Text("Overlay");
      ImGui::SliderFloat("Scanlines##glitch", &g->scanlineAmount, 0.0f, 0.5f,
                         "%.2f");
      ImGui::SliderFloat("Noise##glitch", &g->noiseAmount, 0.0f, 0.3f, "%.2f");
    }
    DrawSectionEnd();
  }
}

static void DrawRetroAsciiArt(EffectConfig *e, const ModSources *modSources,
                              const ImU32 categoryGlow) {
  if (DrawSectionBegin("ASCII Art", categoryGlow, &sectionAsciiArt,
                       e->asciiArt.enabled)) {
    const bool wasEnabled = e->asciiArt.enabled;
    ImGui::Checkbox("Enabled##ascii", &e->asciiArt.enabled);
    if (!wasEnabled && e->asciiArt.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_ASCII_ART);
    }
    if (e->asciiArt.enabled) {
      AsciiArtConfig *aa = &e->asciiArt;

      ModulatableSlider("Cell Size##ascii", &aa->cellSize, "asciiArt.cellSize",
                        "%.0f px", modSources);

      const char *colorModeNames[] = {"Original", "Mono", "CRT Green"};
      ImGui::Combo("Color Mode##ascii", &aa->colorMode, colorModeNames, 3);

      if (aa->colorMode == 1) {
        float fg[3] = {aa->foregroundR, aa->foregroundG, aa->foregroundB};
        if (ImGui::ColorEdit3("Foreground##ascii", fg)) {
          aa->foregroundR = fg[0];
          aa->foregroundG = fg[1];
          aa->foregroundB = fg[2];
        }
        float bg[3] = {aa->backgroundR, aa->backgroundG, aa->backgroundB};
        if (ImGui::ColorEdit3("Background##ascii", bg)) {
          aa->backgroundR = bg[0];
          aa->backgroundG = bg[1];
          aa->backgroundB = bg[2];
        }
      }

      ImGui::Checkbox("Invert##ascii", &aa->invert);
    }
    DrawSectionEnd();
  }
}

static void DrawRetroMatrixRain(EffectConfig *e, const ModSources *modSources,
                                const ImU32 categoryGlow) {
  if (DrawSectionBegin("Matrix Rain", categoryGlow, &sectionMatrixRain,
                       e->matrixRain.enabled)) {
    const bool wasEnabled = e->matrixRain.enabled;
    ImGui::Checkbox("Enabled##matrixrain", &e->matrixRain.enabled);
    if (!wasEnabled && e->matrixRain.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_MATRIX_RAIN);
    }
    if (e->matrixRain.enabled) {
      MatrixRainConfig *mr = &e->matrixRain;

      ImGui::SliderFloat("Cell Size##matrixrain", &mr->cellSize, 4.0f, 32.0f,
                         "%.0f px");
      ModulatableSlider("Rain Speed##matrixrain", &mr->rainSpeed,
                        "matrixRain.rainSpeed", "%.2f", modSources);
      ModulatableSlider("Trail Length##matrixrain", &mr->trailLength,
                        "matrixRain.trailLength", "%.0f", modSources);
      ImGui::SliderInt("Faller Count##matrixrain", &mr->fallerCount, 1, 20);
      ModulatableSlider("Overlay Intensity##matrixrain", &mr->overlayIntensity,
                        "matrixRain.overlayIntensity", "%.2f", modSources);
      ImGui::SliderFloat("Refresh Rate##matrixrain", &mr->refreshRate, 0.1f,
                         5.0f, "%.2f");
      ModulatableSlider("Lead Brightness##matrixrain", &mr->leadBrightness,
                        "matrixRain.leadBrightness", "%.2f", modSources);
      ImGui::Checkbox("Sample##matrixrain", &mr->sampleMode);
    }
    DrawSectionEnd();
  }
}

static void DrawRetroSynthwave(EffectConfig *e, const ModSources *modSources,
                               const ImU32 categoryGlow) {
  if (DrawSectionBegin("Synthwave", categoryGlow, &sectionSynthwave,
                       e->synthwave.enabled)) {
    const bool wasEnabled = e->synthwave.enabled;
    ImGui::Checkbox("Enabled##synthwave", &e->synthwave.enabled);
    if (!wasEnabled && e->synthwave.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_SYNTHWAVE);
    }
    if (e->synthwave.enabled) {
      SynthwaveConfig *sw = &e->synthwave;

      ModulatableSlider("Horizon##synthwave", &sw->horizonY,
                        "synthwave.horizonY", "%.2f", modSources);
      ModulatableSlider("Color Mix##synthwave", &sw->colorMix,
                        "synthwave.colorMix", "%.2f", modSources);

      if (TreeNodeAccented("Palette##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Phase R##synthwave", &sw->palettePhaseR, 0.0f, 1.0f,
                           "%.2f");
        ImGui::SliderFloat("Phase G##synthwave", &sw->palettePhaseG, 0.0f, 1.0f,
                           "%.2f");
        ImGui::SliderFloat("Phase B##synthwave", &sw->palettePhaseB, 0.0f, 1.0f,
                           "%.2f");
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Grid##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Spacing##synthwave", &sw->gridSpacing, 2.0f, 20.0f,
                           "%.1f");
        ImGui::SliderFloat("Line Width##synthwave", &sw->gridThickness, 0.01f,
                           0.1f, "%.3f");
        ModulatableSlider("Opacity##synthwave_grid", &sw->gridOpacity,
                          "synthwave.gridOpacity", "%.2f", modSources);
        ModulatableSlider("Glow##synthwave", &sw->gridGlow,
                          "synthwave.gridGlow", "%.2f", modSources);
        float gridCol[3] = {sw->gridR, sw->gridG, sw->gridB};
        if (ImGui::ColorEdit3("Color##synthwave_grid", gridCol)) {
          sw->gridR = gridCol[0];
          sw->gridG = gridCol[1];
          sw->gridB = gridCol[2];
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Sun Stripes##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Count##synthwave", &sw->stripeCount, 4.0f, 20.0f,
                           "%.0f");
        ImGui::SliderFloat("Softness##synthwave", &sw->stripeSoftness, 0.0f,
                           0.3f, "%.2f");
        ModulatableSlider("Intensity##synthwave_stripe", &sw->stripeIntensity,
                          "synthwave.stripeIntensity", "%.2f", modSources);
        float sunCol[3] = {sw->sunR, sw->sunG, sw->sunB};
        if (ImGui::ColorEdit3("Color##synthwave_sun", sunCol)) {
          sw->sunR = sunCol[0];
          sw->sunG = sunCol[1];
          sw->sunB = sunCol[2];
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Horizon Glow##synthwave", categoryGlow)) {
        ModulatableSlider("Intensity##synthwave_horizon", &sw->horizonIntensity,
                          "synthwave.horizonIntensity", "%.2f", modSources);
        ImGui::SliderFloat("Falloff##synthwave", &sw->horizonFalloff, 5.0f,
                           30.0f, "%.1f");
        float horizonCol[3] = {sw->horizonR, sw->horizonG, sw->horizonB};
        if (ImGui::ColorEdit3("Color##synthwave_horizon", horizonCol)) {
          sw->horizonR = horizonCol[0];
          sw->horizonG = horizonCol[1];
          sw->horizonB = horizonCol[2];
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Animation##synthwave", categoryGlow)) {
        ImGui::SliderFloat("Grid Scroll##synthwave", &sw->gridScrollSpeed, 0.0f,
                           2.0f, "%.2f");
        ImGui::SliderFloat("Stripe Scroll##synthwave", &sw->stripeScrollSpeed,
                           0.0f, 0.5f, "%.3f");
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawRetroCrt(EffectConfig *e, const ModSources *modSources,
                         const ImU32 categoryGlow) {
  if (DrawSectionBegin("CRT", categoryGlow, &sectionCrt, e->crt.enabled)) {
    const bool wasEnabled = e->crt.enabled;
    ImGui::Checkbox("Enabled##crt", &e->crt.enabled);
    if (!wasEnabled && e->crt.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_CRT);
    }
    if (e->crt.enabled) {
      CrtConfig *c = &e->crt;

      if (TreeNodeAccented("Phosphor Mask##crt", categoryGlow)) {
        ImGui::Combo("Mask Mode##crt", &c->maskMode,
                     "Shadow Mask\0Aperture Grille\0");
        ModulatableSlider("Mask Size##crt", &c->maskSize, "crt.maskSize",
                          "%.1f", modSources);
        ModulatableSlider("Mask Intensity##crt", &c->maskIntensity,
                          "crt.maskIntensity", "%.2f", modSources);
        ImGui::SliderFloat("Mask Border##crt", &c->maskBorder, 0.0f, 1.0f,
                           "%.2f");
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Scanlines##crt", categoryGlow)) {
        ModulatableSlider("Scanline Intensity##crt", &c->scanlineIntensity,
                          "crt.scanlineIntensity", "%.2f", modSources);
        ImGui::SliderFloat("Scanline Spacing##crt", &c->scanlineSpacing, 1.0f,
                           8.0f, "%.1f");
        ImGui::SliderFloat("Scanline Sharpness##crt", &c->scanlineSharpness,
                           0.5f, 4.0f, "%.2f");
        ImGui::SliderFloat("Bright Boost##crt", &c->scanlineBrightBoost, 0.0f,
                           1.0f, "%.2f");
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Curvature##crt", categoryGlow)) {
        ImGui::Checkbox("Curvature##crt_enable", &c->curvatureEnabled);
        if (c->curvatureEnabled) {
          ModulatableSlider("Curvature Amount##crt", &c->curvatureAmount,
                            "crt.curvatureAmount", "%.3f", modSources);
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Vignette##crt", categoryGlow)) {
        ImGui::Checkbox("Vignette##crt_enable", &c->vignetteEnabled);
        if (c->vignetteEnabled) {
          ImGui::SliderFloat("Vignette Exponent##crt", &c->vignetteExponent,
                             0.1f, 1.0f, "%.2f");
        }
        TreeNodeAccentedPop();
      }

      if (TreeNodeAccented("Pulse##crt", categoryGlow)) {
        ImGui::Checkbox("Pulse##crt_enable", &c->pulseEnabled);
        if (c->pulseEnabled) {
          ModulatableSlider("Pulse Intensity##crt", &c->pulseIntensity,
                            "crt.pulseIntensity", "%.3f", modSources);
          ModulatableSlider("Pulse Speed##crt", &c->pulseSpeed,
                            "crt.pulseSpeed", "%.1f", modSources);
          ImGui::SliderFloat("Pulse Width##crt", &c->pulseWidth, 20.0f, 200.0f,
                             "%.0f");
        }
        TreeNodeAccentedPop();
      }
    }
    DrawSectionEnd();
  }
}

static void DrawRetroLatticeCrush(EffectConfig *e, const ModSources *modSources,
                                  const ImU32 categoryGlow) {
  if (DrawSectionBegin("Lattice Crush", categoryGlow, &sectionLatticeCrush,
                       e->latticeCrush.enabled)) {
    const bool wasEnabled = e->latticeCrush.enabled;
    ImGui::Checkbox("Enabled##latticecrush", &e->latticeCrush.enabled);
    if (!wasEnabled && e->latticeCrush.enabled) {
      MoveTransformToEnd(&e->transformOrder, TRANSFORM_LATTICE_CRUSH);
    }
    if (e->latticeCrush.enabled) {
      LatticeCrushConfig *lc = &e->latticeCrush;

      ModulatableSlider("Scale##latticecrush", &lc->scale, "latticeCrush.scale",
                        "%.2f", modSources);
      ModulatableSlider("Cell Size##latticecrush", &lc->cellSize,
                        "latticeCrush.cellSize", "%.1f", modSources);
      ImGui::SliderInt("Iterations##latticecrush", &lc->iterations, 4, 64);
      ImGui::Combo("Walk Mode##latticecrush", &lc->walkMode, WALK_MODE_NAMES,
                   WALK_MODE_COUNT);
      ModulatableSlider("Speed##latticecrush", &lc->speed, "latticeCrush.speed",
                        "%.2f", modSources);
      ModulatableSlider("Mix##latticecrush", &lc->mix, "latticeCrush.mix",
                        "%.2f", modSources);
    }
    DrawSectionEnd();
  }
}

void DrawRetroCategory(EffectConfig *e, const ModSources *modSources) {
  const ImU32 categoryGlow = Theme::GetSectionGlow(6);
  DrawCategoryHeader("Retro", categoryGlow);
  DrawRetroPixelation(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawRetroGlitch(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawRetroCrt(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawRetroAsciiArt(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawRetroMatrixRain(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawRetroSynthwave(e, modSources, categoryGlow);
  ImGui::Spacing();
  DrawRetroLatticeCrush(e, modSources, categoryGlow);
}
