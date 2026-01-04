#ifndef MOBIUS_CONFIG_H
#define MOBIUS_CONFIG_H

// Möbius transformation: w = (az + b) / (cz + d)
// Each parameter is a complex number (real, imaginary pair)
struct MobiusConfig {
    bool enabled = false;
    float aReal = 1.0f;   // Identity scale (real)
    float aImag = 0.0f;   // Identity scale (imaginary)
    float bReal = 0.0f;   // No translation (real)
    float bImag = 0.0f;   // No translation (imaginary)
    float cReal = 0.0f;   // No pole (real)
    float cImag = 0.0f;   // No pole (imaginary)
    float dReal = 1.0f;   // Identity denominator (real)
    float dImag = 0.0f;   // Identity denominator (imaginary)
};

#endif // MOBIUS_CONFIG_H
