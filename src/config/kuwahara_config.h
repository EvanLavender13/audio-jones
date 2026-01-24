#ifndef KUWAHARA_CONFIG_H
#define KUWAHARA_CONFIG_H

struct KuwaharaConfig {
    bool enabled = false;
    float radius = 4.0f;       // Kernel radius, cast to int in shader (2-12)
    int quality = 0;           // 0 = basic (4-sector), 1 = generalized (8-sector)
    float sharpness = 8.0f;    // Sector directional selectivity (1.0-18.0, generalized only)
    float hardness = 8.0f;     // Inverse-variance blend aggressiveness (1.0-100.0, generalized only)
};

#endif // KUWAHARA_CONFIG_H
