#ifndef BAND_CONFIG_H
#define BAND_CONFIG_H

struct BandConfig {
    float bassSensitivity = 1.0f;   // 0.5-2.0 multiplier
    float midSensitivity = 1.0f;
    float trebSensitivity = 1.0f;
};

#endif // BAND_CONFIG_H
