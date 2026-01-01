#ifndef INFINITE_ZOOM_CONFIG_H
#define INFINITE_ZOOM_CONFIG_H

struct InfiniteZoomConfig {
    bool enabled = false;
    float speed = 1.0f;       // Zoom speed (0.1-2.0)
    float baseScale = 1.0f;   // Starting scale factor (0.5-2.0)
    float centerX = 0.5f;     // Zoom focal point X (0.0-1.0)
    float centerY = 0.5f;     // Zoom focal point Y (0.0-1.0)
    int layers = 6;           // Layer count (4-8)
    float spiralTurns = 0.0f; // Spiral rotation per zoom cycle (0.0-4.0 turns)
};

#endif // INFINITE_ZOOM_CONFIG_H
