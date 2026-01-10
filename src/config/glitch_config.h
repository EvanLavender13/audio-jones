#ifndef GLITCH_CONFIG_H
#define GLITCH_CONFIG_H

// Glitch: Analog/digital video corruption through UV distortion, chromatic aberration, noise
// Four toggleable sub-modes: CRT barrel distortion, Analog horizontal noise, Digital block displacement, VHS tracking
struct GlitchConfig {
    bool enabled = false;

    // CRT mode: barrel distortion with edge vignette
    bool crtEnabled = false;
    float curvature = 0.1f;        // Barrel strength (0-0.2)
    bool vignetteEnabled = true;

    // Analog mode: horizontal noise distortion with chromatic aberration
    bool analogEnabled = false;
    float analogIntensity = 0.05f; // Horizontal shift amount (0-0.1)
    float aberration = 5.0f;       // Chromatic channel offset in pixels (0-20)

    // Digital mode: block displacement glitches
    bool digitalEnabled = false;
    float blockThreshold = 0.5f;   // Block probability (0.1-0.9). Higher = more blocks.
    float blockOffset = 0.2f;      // Max displacement (0-0.5)

    // VHS mode: tracking bars and scanline noise
    bool vhsEnabled = false;
    float trackingBarIntensity = 0.02f;    // Bar strength (0-0.05)
    float scanlineNoiseIntensity = 0.01f;  // Per-line jitter (0-0.02)
    float colorDriftIntensity = 1.0f;      // R/G channel drift (0-2.0)

    // Overlay: applied when any mode is active
    float scanlineAmount = 0.1f;   // Scanline darkness (0-0.5)
    float noiseAmount = 0.05f;     // White noise (0-0.3)
};

#endif // GLITCH_CONFIG_H
