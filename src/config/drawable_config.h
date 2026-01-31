#ifndef DRAWABLE_CONFIG_H
#define DRAWABLE_CONFIG_H

#include "render/color_config.h"
#include <stdint.h>

typedef enum {
  DRAWABLE_WAVEFORM,
  DRAWABLE_SPECTRUM,
  DRAWABLE_SHAPE,
  DRAWABLE_PARAMETRIC_TRAIL
} DrawableType;
typedef enum { PATH_LINEAR, PATH_CIRCULAR } DrawablePath;

struct DrawableBase {
  bool enabled = true;
  float x = 0.5f;
  float y = 0.5f;
  float rotationSpeed = 0.0f;
  float rotationAngle = 0.0f;
  float opacity = 1.0f;
  uint8_t drawInterval = 0;
  ColorConfig color;
};

struct WaveformData {
  float amplitudeScale = 0.35f;
  float thickness = 2.0f;
  float smoothness = 5.0f;
  float radius = 0.25f;
  float waveformMotionScale = 1.0f;
};

struct SpectrumData {
  float innerRadius = 0.15f;
  float barHeight = 0.25f;
  float barWidth = 0.8f;
  float smoothing = 0.8f;
  float minDb = 10.0f;
  float maxDb = 50.0f;
};

struct ShapeData {
  int sides = 6;
  float width = 0.4f;       // Normalized 0-1, fraction of screen width
  float height = 0.4f;      // Normalized 0-1, fraction of screen height
  bool aspectLocked = true; // UI-only state for linked editing
  bool textured = false;
  float texZoom = 1.0f;
  float texAngle = 0.0f;
  float texBrightness =
      0.9f; // 10% attenuation per frame prevents brightness accumulation
  float texMotionScale =
      1.0f; // Scales zoom/angle deviation from identity (0 = no effect)
};

struct ParametricTrailData {
  // Lissajous parameters
  float speed = 1.0f;       // Phase accumulation rate
  float amplitude = 0.25f;  // Path size (fraction of screen)
  float freqX1 = 3.14159f;  // Primary X frequency
  float freqY1 = 1.0f;      // Primary Y frequency
  float freqX2 = 0.72834f;  // Secondary X frequency (0 = disabled)
  float freqY2 = 2.781374f; // Secondary Y frequency (0 = disabled)
  float offsetX = 0.3f;     // Phase offset for secondary X (radians)
  float offsetY = 3.47912f; // Phase offset for secondary Y (radians)

  // Stroke parameters
  float thickness = 4.0f;  // Stroke width in pixels
  bool roundedCaps = true; // Circle caps at segment endpoints

  // Draw gate: 0 = continuous, >0 = gaps at this rate (Hz)
  float gateFreq = 0.0f;

  // Runtime state (not serialized)
  float phase = 0.0f;      // Accumulated phase (like rotationAccum)
  float prevX = 0.0f;      // Previous cursor X (normalized 0-1)
  float prevY = 0.0f;      // Previous cursor Y (normalized 0-1)
  float prevT = 0.0f;      // Previous color t value (for gradient)
  bool hasPrevPos = false; // Valid previous position exists
};

struct Drawable {
  uint32_t id = 0;
  DrawableType type = DRAWABLE_WAVEFORM;
  DrawablePath path = PATH_CIRCULAR;
  DrawableBase base;
  float rotationAccum = 0.0f; // Runtime accumulator (not saved to preset)
  union {
    WaveformData waveform;
    SpectrumData spectrum;
    ShapeData shape;
    ParametricTrailData parametricTrail;
  };

  Drawable()
      : id(0), type(DRAWABLE_WAVEFORM), path(PATH_CIRCULAR), base(),
        rotationAccum(0.0f), waveform() {}
  Drawable(const Drawable &other)
      : id(other.id), type(other.type), path(other.path), base(other.base),
        rotationAccum(other.rotationAccum) {
    switch (type) {
    case DRAWABLE_WAVEFORM:
      waveform = other.waveform;
      break;
    case DRAWABLE_SPECTRUM:
      spectrum = other.spectrum;
      break;
    case DRAWABLE_SHAPE:
      shape = other.shape;
      break;
    case DRAWABLE_PARAMETRIC_TRAIL:
      parametricTrail = other.parametricTrail;
      break;
    }
  }
  Drawable &operator=(const Drawable &other) {
    id = other.id;
    type = other.type;
    path = other.path;
    base = other.base;
    rotationAccum = other.rotationAccum;
    switch (type) {
    case DRAWABLE_WAVEFORM:
      waveform = other.waveform;
      break;
    case DRAWABLE_SPECTRUM:
      spectrum = other.spectrum;
      break;
    case DRAWABLE_SHAPE:
      shape = other.shape;
      break;
    case DRAWABLE_PARAMETRIC_TRAIL:
      parametricTrail = other.parametricTrail;
      break;
    }
    return *this;
  }
};

#endif // DRAWABLE_CONFIG_H
