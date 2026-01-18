#ifndef LATTICE_FOLD_CONFIG_H
#define LATTICE_FOLD_CONFIG_H

// Lattice Fold: Grid-based tiling symmetry (square, hexagon)
struct LatticeFoldConfig {
    bool enabled = false;
    int cellType = 6;             // Cell geometry: 4=square, 6=hexagon
    float cellScale = 8.0f;       // Cell density (1.0-20.0)
    float rotationSpeed = 0.002f; // Rotation rate (radians/frame)
    float smoothing = 0.0f;       // Blend width at radial fold seams (0.0-0.5, 0 = hard edge)
};

#endif // LATTICE_FOLD_CONFIG_H
