// Fractal fold operations for iterative space-folding effects
#ifndef FOLD_MODE_H
#define FOLD_MODE_H

typedef enum {
  FOLD_BOX = 0,      // Box reflection (abs + offset)
  FOLD_MANDELBOX,    // Box clamp + sphere inversion
  FOLD_SIERPINSKI,   // Tetrahedral plane reflections
  FOLD_MENGER,       // Abs + full 3-axis sort
  FOLD_KALI,         // Sphere inversion (abs/dot)
  FOLD_BURNING_SHIP, // Abs cross-products
  FOLD_COUNT
} FoldMode;

#endif // FOLD_MODE_H
