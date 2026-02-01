#ifndef CIRCUIT_BOARD_CONFIG_H
#define CIRCUIT_BOARD_CONFIG_H

struct CircuitBoardConfig {
  bool enabled = false;
  float patternX = 7.0f;    // X component of pattern constant (1.0-10.0)
  float patternY = 5.0f;    // Y component of pattern constant (1.0-10.0)
  int iterations = 6;       // Recursion depth (3-12)
  float scale = 1.4f;       // Initial folding frequency (0.5-3.0)
  float offset = 0.16f;     // Initial offset between folds (0.05-0.5)
  float scaleDecay = 1.05f; // Scale reduction per iteration (1.01-1.2)
  float strength = 0.5f;    // Warp intensity (0.0-1.0)
  float scrollSpeed = 0.0f; // Animation scroll rate (0.0-2.0)
  bool chromatic = false;   // Per-channel iteration for RGB separation
};

#endif // CIRCUIT_BOARD_CONFIG_H
