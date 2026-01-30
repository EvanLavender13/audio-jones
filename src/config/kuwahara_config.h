#ifndef KUWAHARA_CONFIG_H
#define KUWAHARA_CONFIG_H

struct KuwaharaConfig {
  bool enabled = false;
  float radius = 4.0f; // Kernel radius, cast to int in shader (2-12)
};

#endif // KUWAHARA_CONFIG_H
