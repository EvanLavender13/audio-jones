#ifndef BAND_CONFIG_H
#define BAND_CONFIG_H

struct BandConfig {
    float bassSensitivity = 0.5f;   // 0.5-2.0 multiplier
    float midSensitivity = 0.5f;
    float trebSensitivity = 0.5f;
};

#endif // BAND_CONFIG_H
