#ifndef RENDER_UTILS_H
#define RENDER_UTILS_H

#include "raylib.h"

// HDR prevents banding artifacts in feedback accumulation over many frames
void RenderUtilsInitTextureHDR(RenderTexture2D* tex, int width, int height, const char* logPrefix);

// Draw texture as fullscreen quad with flipped Y for raylib render textures
void RenderUtilsDrawFullscreenQuad(Texture2D texture, int width, int height);

// Clear a render texture to black
void RenderUtilsClearTexture(RenderTexture2D* tex);

#endif // RENDER_UTILS_H
