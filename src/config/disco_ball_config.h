#ifndef DISCO_BALL_CONFIG_H
#define DISCO_BALL_CONFIG_H

#include <stdbool.h>

typedef struct DiscoBallConfig {
    bool enabled = false;
    float sphereRadius = 0.8f;       // Size of ball (0.2-1.5, fraction of screen height)
    float tileSize = 0.12f;          // Facet grid density (0.05-0.3, smaller = more tiles)
    float rotationSpeed = 0.5f;      // Spin rate (radians/sec)
    float bumpHeight = 0.1f;         // Edge bevel depth (0.0-0.2)
    float reflectIntensity = 2.0f;   // Brightness of reflected texture (0.5-5.0)

    // Light projection (spots outside sphere)
    float spotIntensity = 1.0f;       // Background light spot brightness (0.0-3.0)
    float spotFalloff = 1.0f;         // Spot edge softness, higher = softer (0.5-3.0)
    float brightnessThreshold = 0.1f; // Minimum input brightness to project (0.0-0.5)
} DiscoBallConfig;

#endif // DISCO_BALL_CONFIG_H
