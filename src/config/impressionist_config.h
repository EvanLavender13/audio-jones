#ifndef IMPRESSIONIST_CONFIG_H
#define IMPRESSIONIST_CONFIG_H

struct ImpressionistConfig {
    bool enabled = false;
    int splatCount = 8;
    float splatSizeMin = 0.02f;
    float splatSizeMax = 0.12f;
    float strokeFreq = 800.0f;
    float strokeOpacity = 0.7f;
    float outlineStrength = 0.15f;
    float edgeStrength = 4.0f;
    float edgeMaxDarken = 0.15f;
    float grainScale = 400.0f;
    float grainAmount = 0.1f;
    float exposure = 1.0f;
};

#endif // IMPRESSIONIST_CONFIG_H
