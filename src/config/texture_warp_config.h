#ifndef TEXTURE_WARP_CONFIG_H
#define TEXTURE_WARP_CONFIG_H

struct TextureWarpConfig {
    bool enabled = false;
    float strength = 0.05f;   // Warp magnitude per iteration (0.0 to 0.3)
    int iterations = 3;       // Cascade depth (1 to 8)
};

#endif // TEXTURE_WARP_CONFIG_H
