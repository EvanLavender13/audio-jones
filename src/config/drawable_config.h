#ifndef DRAWABLE_CONFIG_H
#define DRAWABLE_CONFIG_H

#include "render/color_config.h"

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
};

struct Drawable {
    DrawableType type = DRAWABLE_WAVEFORM;
    DrawablePath path = PATH_CIRCULAR;
    DrawableBase base;
    union {
        WaveformData waveform;
        SpectrumData spectrum;
        ShapeData shape;
    };

    Drawable() : type(DRAWABLE_WAVEFORM), path(PATH_CIRCULAR), base(), waveform() {}
    Drawable(const Drawable& other) : type(other.type), path(other.path), base(other.base) {
        switch (type) {
        case DRAWABLE_WAVEFORM: waveform = other.waveform; break;
        case DRAWABLE_SPECTRUM: spectrum = other.spectrum; break;
        case DRAWABLE_SHAPE: shape = other.shape; break;
        }
    }
    Drawable& operator=(const Drawable& other) {
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
