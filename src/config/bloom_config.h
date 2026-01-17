#ifndef BLOOM_CONFIG_H
#define BLOOM_CONFIG_H

// Bloom: Dual Kawase blur with soft threshold extraction
// Creates HDR-style soft glow around bright areas

static const int BLOOM_MIP_COUNT = 5;

struct BloomConfig {
    bool enabled = false;
    float threshold = 0.8f;    // Brightness cutoff for extraction (0.0-2.0)
    float knee = 0.5f;         // Soft threshold falloff (0.0-1.0)
    float intensity = 0.5f;    // Final glow strength (0.0-2.0)
    int iterations = 5;        // Mip chain depth (3-5)
};

#endif // BLOOM_CONFIG_H
