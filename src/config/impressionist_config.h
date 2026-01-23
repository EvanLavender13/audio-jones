#ifndef IMPRESSIONIST_CONFIG_H
#define IMPRESSIONIST_CONFIG_H

struct ImpressionistConfig {
    bool enabled = false;
    int splatCount = 11;
    float splatSizeMin = 0.018f;
    float splatSizeMax = 0.1f;
    float strokeFreq = 1200.0f;
    float strokeOpacity = 0.7f;
    float outlineStrength = 1.0f;
    float edgeStrength = 4.0f;
    float edgeMaxDarken = 0.13f;
    float grainScale = 400.0f;
    float grainAmount = 0.1f;
    float exposure = 1.28f;
};

#endif // IMPRESSIONIST_CONFIG_H
