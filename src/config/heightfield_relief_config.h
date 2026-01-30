#ifndef HEIGHTFIELD_RELIEF_CONFIG_H
#define HEIGHTFIELD_RELIEF_CONFIG_H

// Heightfield Relief: Treats texture luminance as a heightfield, computes
// surface normals via Sobel gradient, applies directional Lambertian lighting
// with specular
struct HeightfieldReliefConfig {
  bool enabled = false;
  float intensity = 0.7f;    // Blend strength (0.0-1.0)
  float reliefScale = 0.2f;  // Surface flatness, higher = subtler (0.02-1.0)
  float lightAngle = 0.785f; // Light direction in radians (0-2pi)
  float lightHeight = 0.5f;  // Light elevation (0.1-2.0)
  float shininess = 32.0f;   // Specular exponent (1.0-128.0)
};

#endif // HEIGHTFIELD_RELIEF_CONFIG_H
