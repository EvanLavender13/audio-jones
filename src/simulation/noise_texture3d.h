#ifndef NOISE_TEXTURE3D_H
#define NOISE_TEXTURE3D_H

// 3D tileable noise texture for curl flow and other effects.
// Precomputes simplex noise curl vectors on CPU, stores in RG16F texture.

typedef struct NoiseTexture3D {
    unsigned int textureId;  // GL_TEXTURE_3D handle
    int size;                // Cube dimension (128)
    float frequency;         // Noise frequency used during generation
} NoiseTexture3D;

// Create 3D noise texture with given size and frequency.
// Returns NULL on failure.
NoiseTexture3D* NoiseTexture3DInit(int size, float frequency);

// Clean up texture resources.
void NoiseTexture3DUninit(NoiseTexture3D* noise);

// Get OpenGL texture handle for binding.
unsigned int NoiseTexture3DGetTexture(const NoiseTexture3D* noise);

// Regenerate noise with new frequency.
void NoiseTexture3DRegenerate(NoiseTexture3D* noise, float frequency);

#endif // NOISE_TEXTURE3D_H
