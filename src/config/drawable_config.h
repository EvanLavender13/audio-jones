#ifndef DRAWABLE_CONFIG_H
#define DRAWABLE_CONFIG_H

#include "render/color_config.h"
#include <stdint.h>

typedef enum { DRAWABLE_WAVEFORM, DRAWABLE_SPECTRUM, DRAWABLE_SHAPE } DrawableType;
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
    float width = 0.4f;   // Normalized 0-1, fraction of screen width
    float height = 0.4f;  // Normalized 0-1, fraction of screen height
    bool aspectLocked = true;  // UI-only state for linked editing
    bool textured = false;
    float texZoom = 1.0f;
    float texAngle = 0.0f;
    float texBrightness = 0.9f;  // 10% attenuation per frame prevents brightness accumulation
};

struct Drawable {
    uint32_t id = 0;
    DrawableType type = DRAWABLE_WAVEFORM;
    DrawablePath path = PATH_CIRCULAR;
    DrawableBase base;
    float rotationAccum = 0.0f;  // Runtime accumulator (not saved to preset)
    union {
        WaveformData waveform;
        SpectrumData spectrum;
        ShapeData shape;
    };

    Drawable() : id(0), type(DRAWABLE_WAVEFORM), path(PATH_CIRCULAR), base(), rotationAccum(0.0f), waveform() {}
    Drawable(const Drawable& other) : id(other.id), type(other.type), path(other.path), base(other.base), rotationAccum(other.rotationAccum) {
        switch (type) {
        case DRAWABLE_WAVEFORM: waveform = other.waveform; break;
        case DRAWABLE_SPECTRUM: spectrum = other.spectrum; break;
        case DRAWABLE_SHAPE: shape = other.shape; break;
        }
    }
    Drawable& operator=(const Drawable& other) {
        id = other.id;
        type = other.type;
        path = other.path;
        base = other.base;
        rotationAccum = other.rotationAccum;
        switch (type) {
        case DRAWABLE_WAVEFORM: waveform = other.waveform; break;
        case DRAWABLE_SPECTRUM: spectrum = other.spectrum; break;
        case DRAWABLE_SHAPE: shape = other.shape; break;
        }
        return *this;
    }
};

#endif // DRAWABLE_CONFIG_H
