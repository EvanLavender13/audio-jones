#include "render_utils.h"
#include "rlgl.h"
#include <stddef.h>

void RenderUtilsInitTextureHDR(RenderTexture2D* tex, int width, int height, const char* logPrefix)
{
    tex->id = rlLoadFramebuffer();
    if (tex->id == 0) {
        TraceLog(LOG_WARNING, "%s: Failed to create HDR framebuffer", logPrefix);
        return;
    }

    rlEnableFramebuffer(tex->id);

    tex->texture.id = rlLoadTexture(NULL, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1);
    tex->texture.width = width;
    tex->texture.height = height;
    tex->texture.mipmaps = 1;
    tex->texture.format = RL_PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;

    rlFramebufferAttach(tex->id, tex->texture.id, RL_ATTACHMENT_COLOR_CHANNEL0,
                        RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(tex->id)) {
        TraceLog(LOG_WARNING, "%s: HDR framebuffer incomplete, falling back to standard", logPrefix);
        rlUnloadFramebuffer(tex->id);
        rlUnloadTexture(tex->texture.id);
        *tex = LoadRenderTexture(width, height);
    }

    rlDisableFramebuffer();

    tex->depth.id = 0;

    SetTextureFilter(tex->texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(tex->texture, TEXTURE_WRAP_CLAMP);
    BeginTextureMode(*tex);
    ClearBackground(BLACK);
    EndTextureMode();
}

void RenderUtilsDrawFullscreenQuad(Texture2D texture, int width, int height)
{
    DrawTextureRec(texture, {0, 0, (float)width, (float)-height}, {0, 0}, WHITE);
}

void RenderUtilsClearTexture(RenderTexture2D* tex)
{
    BeginTextureMode(*tex);
    ClearBackground(BLACK);
    EndTextureMode();
}
