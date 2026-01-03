#include "noise_texture3d.h"
#include "external/glad.h"
#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// Permutation table for simplex noise
static const int PERM[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
    8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
    35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
    55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
    18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
    250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
    189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
    172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
    107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

// Gradient vectors for 3D simplex noise
static const float GRAD3[12][3] = {
    {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
    {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
    {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
};

static float Dot3(const float* g, float x, float y, float z)
{
    return g[0] * x + g[1] * y + g[2] * z;
}

static float Floor(float x)
{
    return floorf(x);
}

// 3D simplex noise returning value and gradient
static void SimplexNoise3DGrad(float x, float y, float z, float* outValue, float* outGradX, float* outGradY, float* outGradZ)
{
    const float F3 = 1.0f / 3.0f;
    const float G3 = 1.0f / 6.0f;

    float s = (x + y + z) * F3;
    int i = (int)Floor(x + s);
    int j = (int)Floor(y + s);
    int k = (int)Floor(z + s);

    float t = (float)(i + j + k) * G3;
    float X0 = (float)i - t;
    float Y0 = (float)j - t;
    float Z0 = (float)k - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;

    int i1, j1, k1;
    int i2, j2, k2;

    if (x0 >= y0) {
        if (y0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
        else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
        else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
    } else {
        if (y0 < z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
        else if (x0 < z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
        else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
    }

    float x1 = x0 - (float)i1 + G3;
    float y1 = y0 - (float)j1 + G3;
    float z1 = z0 - (float)k1 + G3;
    float x2 = x0 - (float)i2 + 2.0f * G3;
    float y2 = y0 - (float)j2 + 2.0f * G3;
    float z2 = z0 - (float)k2 + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;

    int gi0 = PERM[ii + PERM[jj + PERM[kk]]] % 12;
    int gi1 = PERM[ii + i1 + PERM[jj + j1 + PERM[kk + k1]]] % 12;
    int gi2 = PERM[ii + i2 + PERM[jj + j2 + PERM[kk + k2]]] % 12;
    int gi3 = PERM[ii + 1 + PERM[jj + 1 + PERM[kk + 1]]] % 12;

    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;
    float gx0 = 0.0f, gy0 = 0.0f, gz0 = 0.0f;
    float gx1 = 0.0f, gy1 = 0.0f, gz1 = 0.0f;
    float gx2 = 0.0f, gy2 = 0.0f, gz2 = 0.0f;
    float gx3 = 0.0f, gy3 = 0.0f, gz3 = 0.0f;

    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 >= 0.0f) {
        float t02 = t0 * t0;
        float t04 = t02 * t02;
        float gdot0 = Dot3(GRAD3[gi0], x0, y0, z0);
        n0 = t04 * gdot0;
        gx0 = t04 * GRAD3[gi0][0] - 8.0f * t0 * t02 * x0 * gdot0;
        gy0 = t04 * GRAD3[gi0][1] - 8.0f * t0 * t02 * y0 * gdot0;
        gz0 = t04 * GRAD3[gi0][2] - 8.0f * t0 * t02 * z0 * gdot0;
    }

    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 >= 0.0f) {
        float t12 = t1 * t1;
        float t14 = t12 * t12;
        float gdot1 = Dot3(GRAD3[gi1], x1, y1, z1);
        n1 = t14 * gdot1;
        gx1 = t14 * GRAD3[gi1][0] - 8.0f * t1 * t12 * x1 * gdot1;
        gy1 = t14 * GRAD3[gi1][1] - 8.0f * t1 * t12 * y1 * gdot1;
        gz1 = t14 * GRAD3[gi1][2] - 8.0f * t1 * t12 * z1 * gdot1;
    }

    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 >= 0.0f) {
        float t22 = t2 * t2;
        float t24 = t22 * t22;
        float gdot2 = Dot3(GRAD3[gi2], x2, y2, z2);
        n2 = t24 * gdot2;
        gx2 = t24 * GRAD3[gi2][0] - 8.0f * t2 * t22 * x2 * gdot2;
        gy2 = t24 * GRAD3[gi2][1] - 8.0f * t2 * t22 * y2 * gdot2;
        gz2 = t24 * GRAD3[gi2][2] - 8.0f * t2 * t22 * z2 * gdot2;
    }

    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 >= 0.0f) {
        float t32 = t3 * t3;
        float t34 = t32 * t32;
        float gdot3 = Dot3(GRAD3[gi3], x3, y3, z3);
        n3 = t34 * gdot3;
        gx3 = t34 * GRAD3[gi3][0] - 8.0f * t3 * t32 * x3 * gdot3;
        gy3 = t34 * GRAD3[gi3][1] - 8.0f * t3 * t32 * y3 * gdot3;
        gz3 = t34 * GRAD3[gi3][2] - 8.0f * t3 * t32 * z3 * gdot3;
    }

    *outValue = 32.0f * (n0 + n1 + n2 + n3);
    *outGradX = 32.0f * (gx0 + gx1 + gx2 + gx3);
    *outGradY = 32.0f * (gy0 + gy1 + gy2 + gy3);
    *outGradZ = 32.0f * (gz0 + gz1 + gz2 + gz3);
}

// Compute curl of noise field at position (returns 2D curl for XY flow)
static void ComputeCurl(float x, float y, float z, float frequency, float* curlX, float* curlY)
{
    float value, gradX, gradY, gradZ;
    SimplexNoise3DGrad(x * frequency, y * frequency, z * frequency, &value, &gradX, &gradY, &gradZ);

    // Curl in 2D: (dFz/dy - dFy/dz, dFx/dz - dFz/dx)
    // For a scalar field, we use perpendicular gradient: (-gradY, gradX)
    *curlX = -gradY * frequency;
    *curlY = gradX * frequency;
}

static void GenerateNoiseTexture(NoiseTexture3D* noise)
{
    const int size = noise->size;
    const float frequency = noise->frequency;
    const int totalSize = size * size * size;

    // RG16F: 2 components, 16-bit float each
    int16_t* data = (int16_t*)malloc(totalSize * 2 * sizeof(int16_t));
    if (data == NULL) {
        return;
    }

    // Generate tileable noise by wrapping coordinates
    for (int z = 0; z < size; z++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                // Normalize to 0-1 range for tileability
                float fx = (float)x / (float)size;
                float fy = (float)y / (float)size;
                float fz = (float)z / (float)size;

                float curlX, curlY;
                ComputeCurl(fx, fy, fz, frequency, &curlX, &curlY);

                // Convert to half-float (simplified: just store as scaled int16)
                // OpenGL will interpret GL_HALF_FLOAT correctly
                int idx = (z * size * size + y * size + x) * 2;

                // Pack as normalized values scaled to use half-float range
                // Using simple linear mapping: value * 1000 fits in half precision
                union { float f; uint32_t u; } ux, uy;
                ux.f = curlX;
                uy.f = curlY;

                // Convert float32 to float16 (simplified)
                uint32_t fx32 = ux.u;
                uint32_t fy32 = uy.u;

                // Extract components
                uint32_t sx = (fx32 >> 31) & 0x1;
                uint32_t ex = (fx32 >> 23) & 0xFF;
                uint32_t mx = fx32 & 0x7FFFFF;

                uint32_t sy = (fy32 >> 31) & 0x1;
                uint32_t ey = (fy32 >> 23) & 0xFF;
                uint32_t my = fy32 & 0x7FFFFF;

                // Convert to half-float
                uint16_t hx, hy;
                if (ex == 0) {
                    hx = (uint16_t)(sx << 15);
                } else if (ex == 255) {
                    hx = (uint16_t)((sx << 15) | 0x7C00 | (mx ? 0x200 : 0));
                } else {
                    int newExp = (int)ex - 127 + 15;
                    if (newExp >= 31) {
                        hx = (uint16_t)((sx << 15) | 0x7C00);
                    } else if (newExp <= 0) {
                        hx = (uint16_t)(sx << 15);
                    } else {
                        hx = (uint16_t)((sx << 15) | (newExp << 10) | (mx >> 13));
                    }
                }

                if (ey == 0) {
                    hy = (uint16_t)(sy << 15);
                } else if (ey == 255) {
                    hy = (uint16_t)((sy << 15) | 0x7C00 | (my ? 0x200 : 0));
                } else {
                    int newExp = (int)ey - 127 + 15;
                    if (newExp >= 31) {
                        hy = (uint16_t)((sy << 15) | 0x7C00);
                    } else if (newExp <= 0) {
                        hy = (uint16_t)(sy << 15);
                    } else {
                        hy = (uint16_t)((sy << 15) | (newExp << 10) | (my >> 13));
                    }
                }

                data[idx + 0] = (int16_t)hx;
                data[idx + 1] = (int16_t)hy;
            }
        }
    }

    // Create or update OpenGL texture
    if (noise->textureId == 0) {
        glGenTextures(1, &noise->textureId);
    }

    glBindTexture(GL_TEXTURE_3D, noise->textureId);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RG16F, size, size, size, 0, GL_RG, GL_HALF_FLOAT, data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glBindTexture(GL_TEXTURE_3D, 0);

    free(data);

    TraceLog(LOG_INFO, "NOISE_TEXTURE_3D: Generated %dx%dx%d texture (freq=%.2f)", size, size, size, frequency);
}

NoiseTexture3D* NoiseTexture3DInit(int size, float frequency)
{
    NoiseTexture3D* noise = (NoiseTexture3D*)calloc(1, sizeof(NoiseTexture3D));
    if (noise == NULL) {
        return NULL;
    }

    noise->size = size;
    noise->frequency = frequency;
    noise->textureId = 0;

    GenerateNoiseTexture(noise);

    if (noise->textureId == 0) {
        free(noise);
        return NULL;
    }

    return noise;
}

void NoiseTexture3DUninit(NoiseTexture3D* noise)
{
    if (noise == NULL) {
        return;
    }
    if (noise->textureId != 0) {
        glDeleteTextures(1, &noise->textureId);
    }
    free(noise);
}

unsigned int NoiseTexture3DGetTexture(const NoiseTexture3D* noise)
{
    if (noise == NULL) {
        return 0;
    }
    return noise->textureId;
}

void NoiseTexture3DRegenerate(NoiseTexture3D* noise, float frequency)
{
    if (noise == NULL) {
        return;
    }
    noise->frequency = frequency;
    GenerateNoiseTexture(noise);
}
