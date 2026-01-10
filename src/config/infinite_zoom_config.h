#ifndef INFINITE_ZOOM_CONFIG_H
#define INFINITE_ZOOM_CONFIG_H

struct InfiniteZoomConfig {
    bool enabled = false;
    float speed = 1.0f;         // Zoom speed (-2.0 to 2.0, negative zooms out)
    float zoomDepth = 3.0f;     // Zoom range in powers of 2 (1.0=2x, 2.0=4x, 3.0=8x)
    int layers = 6;             // Layer count (2-8)
    float spiralAngle = 0.0f;   // Uniform rotation per zoom cycle (radians)
    float spiralTwist = 0.0f;   // Radius-dependent twist via log(r) (radians)
};

#endif // INFINITE_ZOOM_CONFIG_H
