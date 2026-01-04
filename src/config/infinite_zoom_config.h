#ifndef INFINITE_ZOOM_CONFIG_H
#define INFINITE_ZOOM_CONFIG_H

struct InfiniteZoomConfig {
    bool enabled = false;
    float speed = 1.0f;         // Zoom speed (0.1-2.0)
    float zoomDepth = 3.0f;     // Zoom range in powers of 2 (1.0=2x, 2.0=4x, 3.0=8x)
    float focalAmplitude = 0.0f; // Lissajous center offset (UV units, 0 = static center)
    float focalFreqX = 1.0f;    // Lissajous X frequency
    float focalFreqY = 1.5f;    // Lissajous Y frequency
    int layers = 6;             // Layer count (2-8)
    float spiralTurns = 0.0f;   // Uniform rotation per zoom cycle (-4.0 to 4.0 turns)
    float spiralTwist = 0.0f;   // Radius-dependent twist via log(r) (radians)
};

#endif // INFINITE_ZOOM_CONFIG_H
