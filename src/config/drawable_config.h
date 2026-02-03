#ifndef DRAWABLE_CONFIG_H
#define DRAWABLE_CONFIG_H

#include "config/dual_lissajous_config.h"
#include "render/color_config.h"
#include <stdint.h>

typedef enum {
  DRAWABLE_WAVEFORM,
  DRAWABLE_SPECTRUM,
  DRAWABLE_SHAPE,
  DRAWABLE_PARAMETRIC_TRAIL
} DrawableType;
typedef enum { PATH_LINEAR, PATH_CIRCULAR } DrawablePath;

typedef enum {
  TRAIL_SHAPE_CIRCLE = 0,   // sides=32 (smooth circle)
  TRAIL_SHAPE_TRIANGLE = 1, // sides=3
  TRAIL_SHAPE_SQUARE = 2,   // sides=4
  TRAIL_SHAPE_PENTAGON = 3, // sides=5
  TRAIL_SHAPE_HEXAGON = 4,  // sides=6
} TrailShapeType;

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
  float colorShift = 0.0f;      // Static color offset (radians)
  float colorShiftSpeed = 0.0f; // Color cycling rate (radians/sec)
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
  // Lissajous motion parameters
  DualLissajousConfig lissajous = {
      .amplitude = 0.25f,
      .motionSpeed = 1.0f,
      .freqX1 = 3.14159f,
      .freqY1 = 1.0f,
      .freqX2 = 0.72834f,
      .freqY2 = 2.781374f,
      .offsetX2 = 0.3f,
      .offsetY2 = 3.47912f,
      .phase = 0.0f,
  };

  // Shape parameters
  TrailShapeType shapeType = TRAIL_SHAPE_CIRCLE;
  float size = 8.0f;  // Shape diameter in pixels
  bool filled = true; // true=filled, false=outline

  // Draw gate: 0 = continuous, >0 = gaps at this rate (Hz)
  float gateFreq = 0.0f;
};

struct Drawable {
  uint32_t id = 0;
  DrawableType type = DRAWABLE_WAVEFORM;
  DrawablePath path = PATH_CIRCULAR;
  DrawableBase base;
  float rotationAccum = 0.0f;   // Runtime accumulator (not saved to preset)
  float colorShiftAccum = 0.0f; // Color shift accumulator (not saved to preset)
  union {
    WaveformData waveform;
    SpectrumData spectrum;
    ShapeData shape;
    ParametricTrailData parametricTrail;
  };

  Drawable()
      : id(0), type(DRAWABLE_WAVEFORM), path(PATH_CIRCULAR), base(),
        rotationAccum(0.0f), colorShiftAccum(0.0f), waveform() {}
  Drawable(const Drawable &other)
      : id(other.id), type(other.type), path(other.path), base(other.base),
        rotationAccum(other.rotationAccum),
        colorShiftAccum(other.colorShiftAccum) {
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
    colorShiftAccum = other.colorShiftAccum;
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
