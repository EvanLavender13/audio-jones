#ifndef OIL_PAINT_CONFIG_H
#define OIL_PAINT_CONFIG_H

// OilPaint: Painterly style effect using 4-sector Kuwahara filter
// Smooths flat regions while preserving hard edges with characteristic brush strokes
struct OilPaintConfig {
    bool enabled = false;
    float radius = 4.0f;     // Filter kernel radius (2.0-8.0)
    float sharpness = 4.0f;  // Sector blending sharpness (0.1-8.0, higher = crisper edges)
};

#endif // OIL_PAINT_CONFIG_H
