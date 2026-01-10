#ifndef DROSTE_ZOOM_CONFIG_H
#define DROSTE_ZOOM_CONFIG_H

struct DrosteZoomConfig {
    bool enabled = false;
    float speed = 1.0f;         // Animation speed (-2.0 to 2.0, negative zooms in)
    float scale = 2.5f;         // Ratio between recursive copies (1.5 to 10.0)
    float spiralAngle = 0.0f;   // Additional rotation per cycle (radians)
    float twist = 0.0f;         // Radius-dependent twist beyond natural alpha (-1.0 to 1.0)
    float innerRadius = 0.0f;   // Mask center singularity (0.0 to 0.5)
    int branches = 1;           // Number of spiral arms (1 to 8)
};

#endif // DROSTE_ZOOM_CONFIG_H
