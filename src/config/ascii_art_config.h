#ifndef ASCII_ART_CONFIG_H
#define ASCII_ART_CONFIG_H

// ASCII Art: Converts frame to text characters based on luminance
// Darker regions display denser characters (@, #), lighter regions display
// sparser ones (., space)
struct AsciiArtConfig {
  bool enabled = false;
  float cellSize =
      8.0f; // Cell size in pixels (4-32). Larger = fewer, bigger characters.
  int colorMode = 0;        // 0 = Original colors, 1 = Mono, 2 = CRT green
  float foregroundR = 0.0f; // Mono mode foreground color
  float foregroundG = 1.0f;
  float foregroundB = 0.0f;
  float backgroundR = 0.0f; // Mono mode background color
  float backgroundG = 0.02f;
  float backgroundB = 0.0f;
  bool invert = false; // Swap light/dark character mapping
};

#endif // ASCII_ART_CONFIG_H
