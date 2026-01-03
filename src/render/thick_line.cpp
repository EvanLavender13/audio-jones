#include "thick_line.h"
#include "rlgl.h"
#include <math.h>

// Miter limit: when miter length exceeds thickness * this factor, use bevel
#define MITER_LIMIT 2.0f

struct ThickLinePoint {
    Vector2 pos;
    Color color;
};

static ThickLinePoint pointBuffer[THICK_LINE_MAX_POINTS];
static int pointCount = 0;
static float lineThickness = 1.0f;

static float Vec2Length(Vector2 v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

static Vector2 Vec2Normalize(Vector2 v)
{
    const float len = Vec2Length(v);
    if (len < 0.0001f) {
        return (Vector2){ 0.0f, 0.0f };
    }
    return (Vector2){ v.x / len, v.y / len };
}

static float Vec2Dot(Vector2 a, Vector2 b)
{
    return a.x * b.x + a.y * b.y;
}

// Perpendicular (90 degrees counter-clockwise)
static Vector2 Vec2Perp(Vector2 v)
{
    return (Vector2){ -v.y, v.x };
}

// Compute direction from p0 to p1
static Vector2 SegmentDir(Vector2 p0, Vector2 p1)
{
    return Vec2Normalize((Vector2){ p1.x - p0.x, p1.y - p0.y });
}

// Compute miter vector and length at a corner between two segments
// Returns miter length (negative if degenerate)
static float ComputeMiter(Vector2 dirA, Vector2 dirB, float halfThick, Vector2* outMiter)
{
    // Tangent is bisector of the two directions
    Vector2 tangent = Vec2Normalize((Vector2){ dirA.x + dirB.x, dirA.y + dirB.y });

    // Miter is perpendicular to tangent
    Vector2 miter = Vec2Perp(tangent);

    // Normal of first segment
    Vector2 normalA = Vec2Perp(dirA);

    // Miter length = halfThickness / dot(miter, normalA)
    const float d = Vec2Dot(miter, normalA);
    if (fabsf(d) < 0.0001f) {
        // Segments are nearly parallel, degenerate case
        *outMiter = normalA;
        return -1.0f;
    }

    const float miterLen = halfThick / d;
    *outMiter = miter;
    return miterLen;
}

// Emit a single triangle
static void EmitTriangle(Vector2 v0, Vector2 v1, Vector2 v2, Color c)
{
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex2f(v0.x, v0.y);
    rlVertex2f(v1.x, v1.y);
    rlVertex2f(v2.x, v2.y);
}

// Emit a quad as two triangles (v0-v1-v2-v3 in order: left0, right0, right1, left1)
static void EmitQuad(Vector2 left0, Vector2 right0, Vector2 right1, Vector2 left1, Color c0, Color c1)
{
    // Triangle 1: left0, right0, right1
    rlColor4ub(c0.r, c0.g, c0.b, c0.a);
    rlVertex2f(left0.x, left0.y);
    rlVertex2f(right0.x, right0.y);
    rlColor4ub(c1.r, c1.g, c1.b, c1.a);
    rlVertex2f(right1.x, right1.y);

    // Triangle 2: left0, right1, left1
    rlColor4ub(c0.r, c0.g, c0.b, c0.a);
    rlVertex2f(left0.x, left0.y);
    rlColor4ub(c1.r, c1.g, c1.b, c1.a);
    rlVertex2f(right1.x, right1.y);
    rlVertex2f(left1.x, left1.y);
}

void ThickLineBegin(float thickness)
{
    pointCount = 0;
    lineThickness = thickness;
}

void ThickLineVertex(Vector2 pos, Color color)
{
    if (pointCount < THICK_LINE_MAX_POINTS) {
        pointBuffer[pointCount].pos = pos;
        pointBuffer[pointCount].color = color;
        pointCount++;
    }
}

void ThickLineEnd(bool closed)
{
    if (pointCount < 2) {
        return;
    }

    const float halfThick = lineThickness * 0.5f;
    const float miterLimit = lineThickness * MITER_LIMIT;

    // Compute left/right offset vertices for each point
    // For interior points, use miter join; for endpoints (open), use segment normal
    static Vector2 leftVerts[THICK_LINE_MAX_POINTS];
    static Vector2 rightVerts[THICK_LINE_MAX_POINTS];

    const int n = pointCount;

    for (int i = 0; i < n; i++) {
        Vector2 dirPrev, dirNext;
        bool hasPrev = false;
        bool hasNext = false;

        // Previous segment direction
        if (i > 0) {
            dirPrev = SegmentDir(pointBuffer[i - 1].pos, pointBuffer[i].pos);
            hasPrev = true;
        } else if (closed) {
            dirPrev = SegmentDir(pointBuffer[n - 1].pos, pointBuffer[i].pos);
            hasPrev = true;
        }

        // Next segment direction
        if (i < n - 1) {
            dirNext = SegmentDir(pointBuffer[i].pos, pointBuffer[i + 1].pos);
            hasNext = true;
        } else if (closed) {
            dirNext = SegmentDir(pointBuffer[i].pos, pointBuffer[0].pos);
            hasNext = true;
        }

        Vector2 offset;

        if (hasPrev && hasNext) {
            // Interior point: compute miter
            Vector2 miter;
            const float miterLen = ComputeMiter(dirPrev, dirNext, halfThick, &miter);

            if (miterLen < 0.0f || fabsf(miterLen) > miterLimit) {
                // Degenerate or too sharp: use average normal (bevel-like)
                Vector2 normalPrev = Vec2Perp(dirPrev);
                Vector2 normalNext = Vec2Perp(dirNext);
                offset = Vec2Normalize((Vector2){
                    normalPrev.x + normalNext.x,
                    normalPrev.y + normalNext.y
                });
                offset.x *= halfThick;
                offset.y *= halfThick;
            } else {
                offset.x = miter.x * miterLen;
                offset.y = miter.y * miterLen;
            }
        } else if (hasNext) {
            // Start point: use next segment's normal
            Vector2 normal = Vec2Perp(dirNext);
            offset.x = normal.x * halfThick;
            offset.y = normal.y * halfThick;
        } else if (hasPrev) {
            // End point: use previous segment's normal
            Vector2 normal = Vec2Perp(dirPrev);
            offset.x = normal.x * halfThick;
            offset.y = normal.y * halfThick;
        } else {
            // Single point (shouldn't happen with n >= 2)
            offset.x = halfThick;
            offset.y = 0.0f;
        }

        const Vector2 pos = pointBuffer[i].pos;
        leftVerts[i] = (Vector2){ pos.x + offset.x, pos.y + offset.y };
        rightVerts[i] = (Vector2){ pos.x - offset.x, pos.y - offset.y };
    }

    // Emit triangles
    rlBegin(RL_TRIANGLES);

    const int segCount = closed ? n : (n - 1);
    for (int i = 0; i < segCount; i++) {
        const int next = (i + 1) % n;

        EmitQuad(
            leftVerts[i], rightVerts[i],
            rightVerts[next], leftVerts[next],
            pointBuffer[i].color, pointBuffer[next].color
        );
    }

    rlEnd();
}
