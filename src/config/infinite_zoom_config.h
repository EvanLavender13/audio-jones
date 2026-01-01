#ifndef INFINITE_ZOOM_CONFIG_H
#define INFINITE_ZOOM_CONFIG_H

struct InfiniteZoomConfig {
    bool enabled = false;
    float speed = 1.0f;         // Zoom speed (0.1-2.0)
    float baseScale = 1.0f;     // Starting scale factor (0.5-2.0)
    float focalAmplitude = 0.0f; // Lissajous center offset (UV units, 0 = static center)
    float focalFreqX = 1.0f;    // Lissajous X frequency
    float focalFreqY = 1.5f;    // Lissajous Y frequency
    int layers = 6;             // Layer count (4-8)
    float spiralTurns = 0.0f;   // Spiral rotation per zoom cycle (0.0-4.0 turns)
};

#endif // INFINITE_ZOOM_CONFIG_H
