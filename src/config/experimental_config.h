#ifndef EXPERIMENTAL_CONFIG_H
#define EXPERIMENTAL_CONFIG_H

struct ExperimentalConfig {
    float halfLife = 0.5f;           // Trail persistence in seconds
    float zoomFactor = 0.995f;       // Subtle zoom toward center per frame
    float injectionOpacity = 0.3f;   // Waveform blend strength (0.05 min, 1 = full)
};

#endif // EXPERIMENTAL_CONFIG_H
