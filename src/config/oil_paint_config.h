#ifndef OIL_PAINT_CONFIG_H
#define OIL_PAINT_CONFIG_H

// OilPaint: Geometry-based brush strokes that simulate layered paint application
// Traces stroke paths along image gradients, accumulating color through multiple passes
struct OilPaintConfig {
    bool enabled = false;
    float brushSize = 1.0f;      // Stroke width relative to base grid cell (0.5-3.0)
    float strokeBend = -1.0f;    // Curvature bias follows or opposes gradient direction (-2.0 to 2.0)
    float specular = 0.15f;      // Surface sheen from simulated paint thickness variation (0.0-1.0)
    int layers = 8;              // Overlapping passes blend like wet-on-wet technique (3-11)
};

#endif // OIL_PAINT_CONFIG_H
