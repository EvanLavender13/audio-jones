#include "shape.h"
#include "post_effect.h"
#include "draw_utils.h"
#include "raylib.h"
#include "rlgl.h"
#include <math.h>

#define MAX_SHAPE_SIDES 32

struct ShapeGeometry {
    int sides;
    float rotation;
    float centerX;
    float centerY;
    float radius;
};

// Returns false if shape should not be drawn (invalid sides)
static bool ShapeCalcGeometry(const RenderContext* ctx, const Drawable* d, uint64_t globalTick, ShapeGeometry* out)
{
    out->sides = d->shape.sides;
    if (out->sides < 3 || out->sides > MAX_SHAPE_SIDES) {
        return false;
    }

    out->rotation = d->base.rotationOffset + (d->base.rotationSpeed * (float)globalTick);
    out->centerX = d->base.x * ctx->screenW;
    out->centerY = d->base.y * ctx->screenH;
    out->radius = d->shape.size * ctx->minDim;
    return true;
}

void ShapeDrawSolid(const RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity)
{
    ShapeGeometry geo;
    if (!ShapeCalcGeometry(ctx, d, globalTick, &geo)) {
        return;
    }

    const Vector2 center = { geo.centerX, geo.centerY };

    for (int i = 0; i < geo.sides; i++) {
        const float t = (float)i / geo.sides;
        const Color triColor = ColorFromConfig(&d->base.color, t, opacity);

        const float angle1 = (2.0f * PI * i / geo.sides) + geo.rotation;
        const float angle2 = (2.0f * PI * (i + 1) / geo.sides) + geo.rotation;

        const Vector2 v1 = { geo.centerX + cosf(angle1) * geo.radius, geo.centerY + sinf(angle1) * geo.radius };
        const Vector2 v2 = { geo.centerX + cosf(angle2) * geo.radius, geo.centerY + sinf(angle2) * geo.radius };

        DrawTriangle(center, v2, v1, triColor);
    }
}

void ShapeDrawTextured(const RenderContext* ctx, const Drawable* d, uint64_t globalTick, float opacity)
{
    if (ctx->postEffect == NULL) {
        return;
    }

    ShapeGeometry geo;
    if (!ShapeCalcGeometry(ctx, d, globalTick, &geo)) {
        return;
    }

    const float texZoom = d->shape.texZoom;
    const float texAngle = d->shape.texAngle;

    const unsigned char alpha = (unsigned char)(opacity * 255.0f);
    const Color tint = { 255, 255, 255, alpha };

    PostEffect* pe = ctx->postEffect;
    SetShaderValue(pe->shapeTextureShader, pe->shapeTexZoomLoc, &texZoom, SHADER_UNIFORM_FLOAT);
    SetShaderValue(pe->shapeTextureShader, pe->shapeTexAngleLoc, &texAngle, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(pe->shapeTextureShader);

    // rlSetTexture must be called AFTER rlBegin - mode switch resets texture
    // See: https://github.com/raysan5/raylib/issues/4347
    rlBegin(RL_TRIANGLES);
    rlSetTexture(ctx->accumTexture.id);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    for (int i = 0; i < geo.sides; i++) {
        const int next = (i + 1) % geo.sides;

        const float angle1 = (2.0f * PI * i / geo.sides) + geo.rotation;
        const float angle2 = (2.0f * PI * next / geo.sides) + geo.rotation;

        const float cos1 = cosf(angle1);
        const float sin1 = sinf(angle1);
        const float cos2 = cosf(angle2);
        const float sin2 = sinf(angle2);

        const float x1 = geo.centerX + cos1 * geo.radius;
        const float y1 = geo.centerY + sin1 * geo.radius;
        const float x2 = geo.centerX + cos2 * geo.radius;
        const float y2 = geo.centerY + sin2 * geo.radius;

        // UV mapping: unit circle to [0,1] centered at 0.5, V flipped for render texture
        const float u1 = cos1 * 0.5f + 0.5f;
        const float v1 = 1.0f - (sin1 * 0.5f + 0.5f);
        const float u2 = cos2 * 0.5f + 0.5f;
        const float v2 = 1.0f - (sin2 * 0.5f + 0.5f);

        rlTexCoord2f(0.5f, 0.5f);
        rlVertex2f(geo.centerX, geo.centerY);

        rlTexCoord2f(u2, v2);
        rlVertex2f(x2, y2);

        rlTexCoord2f(u1, v1);
        rlVertex2f(x1, y1);
    }

    rlEnd();
    rlSetTexture(0);

    EndShaderMode();
}
