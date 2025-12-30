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
    float rotationOffset = 0.0f;
    float feedbackPhase = 1.0f;
    ColorConfig color;
};

struct WaveformData {
    float amplitudeScale = 0.35f;
    int thickness = 2;
    float smoothness = 5.0f;
    float radius = 0.25f;
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
    float size = 0.2f;
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
    union {
        WaveformData waveform;
        SpectrumData spectrum;
        ShapeData shape;
    };

    Drawable() : id(0), type(DRAWABLE_WAVEFORM), path(PATH_CIRCULAR), base(), waveform() {}
    Drawable(const Drawable& other) : id(other.id), type(other.type), path(other.path), base(other.base) {
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
        switch (type) {
        case DRAWABLE_WAVEFORM: waveform = other.waveform; break;
        case DRAWABLE_SPECTRUM: spectrum = other.spectrum; break;
        case DRAWABLE_SHAPE: shape = other.shape; break;
        }
        return *this;
    }
};

#endif // DRAWABLE_CONFIG_H
