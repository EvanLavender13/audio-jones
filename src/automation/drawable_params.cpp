#include "drawable_params.h"
#include "config/drawable_config.h"
#include "modulation_engine.h"
#include "ui/ui_units.h"
#include <stdio.h>

void DrawableParamsRegister(Drawable *d) {
  char paramId[64];

  // Register x param
  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.x", d->id);
  ModEngineRegisterParam(paramId, &d->base.x, -1.0f, 2.0f);

  // Register y param
  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.y", d->id);
  ModEngineRegisterParam(paramId, &d->base.y, -1.0f, 2.0f);

  // Register rotationSpeed param
  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.rotationSpeed", d->id);
  ModEngineRegisterParam(paramId, &d->base.rotationSpeed, -ROTATION_SPEED_MAX,
                         ROTATION_SPEED_MAX);

  // Register rotationAngle param
  (void)snprintf(paramId, sizeof(paramId), "drawable.%u.rotationAngle", d->id);
  ModEngineRegisterParam(paramId, &d->base.rotationAngle, -ROTATION_OFFSET_MAX,
                         ROTATION_OFFSET_MAX);

  // Waveform-specific params
  if (d->type == DRAWABLE_WAVEFORM) {
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.radius", d->id);
    ModEngineRegisterParam(paramId, &d->waveform.radius, 0.05f, 0.45f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.amplitudeScale",
                   d->id);
    ModEngineRegisterParam(paramId, &d->waveform.amplitudeScale, 0.05f, 0.5f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.thickness", d->id);
    ModEngineRegisterParam(paramId, &d->waveform.thickness, 1.0f, 25.0f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.smoothness", d->id);
    ModEngineRegisterParam(paramId, &d->waveform.smoothness, 0.0f, 100.0f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.waveformMotionScale",
                   d->id);
    ModEngineRegisterParam(paramId, &d->waveform.waveformMotionScale, 0.01f,
                           1.0f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.colorShift", d->id);
    ModEngineRegisterParam(paramId, &d->waveform.colorShift, 0.0f, 2.0f * PI);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.colorShiftSpeed",
                   d->id);
    ModEngineRegisterParam(paramId, &d->waveform.colorShiftSpeed, -2.0f * PI,
                           2.0f * PI);
  }

  // Spectrum-specific params
  if (d->type == DRAWABLE_SPECTRUM) {
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.colorShift", d->id);
    ModEngineRegisterParam(paramId, &d->spectrum.colorShift, 0.0f, 2.0f * PI);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.colorShiftSpeed",
                   d->id);
    ModEngineRegisterParam(paramId, &d->spectrum.colorShiftSpeed, -2.0f * PI,
                           2.0f * PI);
  }

  // Shape-specific params
  if (d->type == DRAWABLE_SHAPE) {
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.texAngle", d->id);
    ModEngineRegisterParam(paramId, &d->shape.texAngle, -ROTATION_OFFSET_MAX,
                           ROTATION_OFFSET_MAX);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.texMotionScale",
                   d->id);
    ModEngineRegisterParam(paramId, &d->shape.texMotionScale, 0.01f, 1.0f);
  }

  // Parametric trail params
  if (d->type == DRAWABLE_PARAMETRIC_TRAIL) {
    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.lissajous.amplitude",
                   d->id);
    ModEngineRegisterParam(paramId, &d->parametricTrail.lissajous.amplitude,
                           0.05f, 0.5f);

    (void)snprintf(paramId, sizeof(paramId),
                   "drawable.%u.lissajous.motionSpeed", d->id);
    ModEngineRegisterParam(paramId, &d->parametricTrail.lissajous.motionSpeed,
                           0.1f, 10.0f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.size", d->id);
    ModEngineRegisterParam(paramId, &d->parametricTrail.size, 1.0f, 100.0f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.gateFreq", d->id);
    ModEngineRegisterParam(paramId, &d->parametricTrail.gateFreq, 0.0f, 20.0f);

    (void)snprintf(paramId, sizeof(paramId), "drawable.%u.strokeThickness",
                   d->id);
    ModEngineRegisterParam(paramId, &d->parametricTrail.strokeThickness, 1.0f,
                           10.0f);
  }
}

void DrawableParamsUnregister(uint32_t id) {
  char prefix[64];
  (void)snprintf(prefix, sizeof(prefix), "drawable.%u.", id);
  ModEngineRemoveRoutesMatching(prefix);
  ModEngineRemoveParamsMatching(prefix);
}

void DrawableParamsSyncAll(Drawable *arr, int count) {
  for (int i = 0; i < count; i++) {
    DrawableParamsRegister(&arr[i]);
  }
}
