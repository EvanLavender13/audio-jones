#ifndef UI_UNITS_H
#define UI_UNITS_H

#include "config/constants.h"
#include "config/dual_lissajous_config.h"
#include "imgui.h"
#include "ui/modulatable_slider.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define TICK_RATE_HZ 20.0f
#define SECONDS_PER_TICK 0.05f
#define MAX_DRAW_INTERVAL_SECONDS 5.0f

inline bool SliderAngleDeg(const char *label, float *radians, float minDeg,
                           float maxDeg, const char *format = "%.1f °") {
  float degrees = *radians * RAD_TO_DEG;
  if (ImGui::SliderFloat(label, &degrees, minDeg, maxDeg, format)) {
    *radians = degrees * DEG_TO_RAD;
    return true;
  }
  return false;
}

inline bool ModulatableSliderAngleDeg(const char *label, float *radians,
                                      const char *paramId,
                                      const ModSources *sources,
                                      const char *format = "%.1f °") {
  return ModulatableSlider(label, radians, paramId, format, sources,
                           RAD_TO_DEG);
}

inline bool SliderSpeedDeg(const char *label, float *radians, float minDeg,
                           float maxDeg, const char *format = "%.1f °/s") {
  float degrees = *radians * RAD_TO_DEG;
  if (ImGui::SliderFloat(label, &degrees, minDeg, maxDeg, format)) {
    *radians = degrees * DEG_TO_RAD;
    return true;
  }
  return false;
}

inline bool ModulatableSliderSpeedDeg(const char *label, float *radians,
                                      const char *paramId,
                                      const ModSources *sources,
                                      const char *format = "%.1f °/s") {
  return ModulatableSlider(label, radians, paramId, format, sources,
                           RAD_TO_DEG);
}

// Modulatable slider with logarithmic scale (useful for 0.01-1.0 ranges)
inline bool ModulatableSliderLog(const char *label, float *value,
                                 const char *paramId, const char *format,
                                 const ModSources *sources) {
  return ModulatableSlider(label, value, paramId, format, sources, 1.0f,
                           ImGuiSliderFlags_Logarithmic);
}

// Modulatable slider that displays and snaps to integer values
// Value stored as float for modulation compatibility, but UI shows integers
inline bool ModulatableSliderInt(const char *label, float *value,
                                 const char *paramId,
                                 const ModSources *sources) {
  *value = roundf(*value);
  bool changed = ModulatableSlider(label, value, paramId, "%.0f", sources);
  if (changed) {
    *value = roundf(*value);
  }
  return changed;
}

// Draw interval slider: displays seconds (0-5.0), stores ticks (0-100) at 20 Hz
inline bool SliderDrawInterval(const char *label, uint8_t *ticks) {
  float seconds = *ticks * SECONDS_PER_TICK;
  const char *format = (*ticks == 0) ? "Every frame" : "%.2f s";
  if (ImGui::SliderFloat(label, &seconds, 0.0f, MAX_DRAW_INTERVAL_SECONDS,
                         format)) {
    *ticks = (uint8_t)(seconds * TICK_RATE_HZ + 0.5f);
    return true;
  }
  return false;
}

// Draw lissajous motion controls (amplitude, motionSpeed, frequencies, offsets)
// idSuffix: ImGui ID suffix (e.g., "cym_liss") - pass NULL to omit
// paramPrefix: modulation param prefix (e.g., "cymatics.lissajous") - pass NULL
// to disable modulation freqMax: max frequency for sliders (0.2 for slow, 5.0
// for fast)
inline void DrawLissajousControls(DualLissajousConfig *cfg,
                                  const char *idSuffix, const char *paramPrefix,
                                  const ModSources *modSources,
                                  float freqMax = 5.0f, bool show3D = false,
                                  float freqMin = 0.0f) {
  char label[64];
  char paramId[64];
  const char *suffix = idSuffix ? idSuffix : "liss";

  // -- Amplitudes --
  (void)snprintf(label, sizeof(label), "Amplitude##%s", suffix);
  if (paramPrefix && modSources) {
    (void)snprintf(paramId, sizeof(paramId), "%s.amplitude", paramPrefix);
    ModulatableSlider(label, &cfg->amplitude, paramId, "%.2f", modSources);
  } else {
    ImGui::SliderFloat(label, &cfg->amplitude, 0.0f, 0.5f, "%.2f");
  }
  if (show3D) {
    (void)snprintf(label, sizeof(label), "Amplitude Z##%s", suffix);
    if (paramPrefix && modSources) {
      (void)snprintf(paramId, sizeof(paramId), "%s.amplitudeZ", paramPrefix);
      ModulatableSlider(label, &cfg->amplitudeZ, paramId, "%.2f", modSources);
    } else {
      ImGui::SliderFloat(label, &cfg->amplitudeZ, 0.0f, 0.5f, "%.2f");
    }
  }

  // -- Motion Speed --
  (void)snprintf(label, sizeof(label), "Motion Speed##%s", suffix);
  if (paramPrefix && modSources) {
    (void)snprintf(paramId, sizeof(paramId), "%s.motionSpeed", paramPrefix);
    ModulatableSlider(label, &cfg->motionSpeed, paramId, "%.2f", modSources);
  } else {
    ImGui::SliderFloat(label, &cfg->motionSpeed, 0.0f, 10.0f, "%.2f");
  }

  // -- Primary frequencies (X, Y, Z) --
  (void)snprintf(label, sizeof(label), "Freq X##%s", suffix);
  ImGui::SliderFloat(label, &cfg->freqX1, freqMin, freqMax, "%.2f Hz");
  (void)snprintf(label, sizeof(label), "Freq Y##%s", suffix);
  ImGui::SliderFloat(label, &cfg->freqY1, freqMin, freqMax, "%.2f Hz");
  if (show3D) {
    (void)snprintf(label, sizeof(label), "Freq Z##%s", suffix);
    ImGui::SliderFloat(label, &cfg->freqZ1, freqMin, freqMax, "%.2f Hz");
  }

  // -- Secondary frequencies (X2, Y2, Z2) --
  (void)snprintf(label, sizeof(label), "Freq X2##%s", suffix);
  ImGui::SliderFloat(label, &cfg->freqX2, 0.0f, freqMax, "%.2f Hz");
  (void)snprintf(label, sizeof(label), "Freq Y2##%s", suffix);
  ImGui::SliderFloat(label, &cfg->freqY2, 0.0f, freqMax, "%.2f Hz");
  if (show3D) {
    (void)snprintf(label, sizeof(label), "Freq Z2##%s", suffix);
    ImGui::SliderFloat(label, &cfg->freqZ2, 0.0f, freqMax, "%.2f Hz");
  }

  // -- Offsets (shown when any secondary freq is active) --
  bool hasSecondary = cfg->freqX2 > 0.0f || cfg->freqY2 > 0.0f ||
                      (show3D && cfg->freqZ2 > 0.0f);
  if (hasSecondary) {
    (void)snprintf(label, sizeof(label), "Offset X2##%s", suffix);
    if (paramPrefix && modSources) {
      (void)snprintf(paramId, sizeof(paramId), "%s.offsetX2", paramPrefix);
      ModulatableSliderAngleDeg(label, &cfg->offsetX2, paramId, modSources);
    } else {
      SliderAngleDeg(label, &cfg->offsetX2, -180.0f, 180.0f);
    }
    (void)snprintf(label, sizeof(label), "Offset Y2##%s", suffix);
    if (paramPrefix && modSources) {
      (void)snprintf(paramId, sizeof(paramId), "%s.offsetY2", paramPrefix);
      ModulatableSliderAngleDeg(label, &cfg->offsetY2, paramId, modSources);
    } else {
      SliderAngleDeg(label, &cfg->offsetY2, -180.0f, 180.0f);
    }
    if (show3D) {
      (void)snprintf(label, sizeof(label), "Offset Z2##%s", suffix);
      if (paramPrefix && modSources) {
        (void)snprintf(paramId, sizeof(paramId), "%s.offsetZ2", paramPrefix);
        ModulatableSliderAngleDeg(label, &cfg->offsetZ2, paramId, modSources);
      } else {
        SliderAngleDeg(label, &cfg->offsetZ2, -180.0f, 180.0f);
      }
    }
  }
}

#endif // UI_UNITS_H
