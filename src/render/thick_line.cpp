#include "thick_line.h"
#include "raymath.h"
#include "rlgl.h"

struct ThickLinePoint {
  Vector2 pos;
  Color color;
};

static ThickLinePoint pointBuffer[THICK_LINE_MAX_POINTS];
static int pointCount = 0;
static float lineThickness = 1.0f;

static Vector2 Vec2Perp(Vector2 v) { return Vector2{-v.y, v.x}; }

void ThickLineBegin(float thickness) {
  pointCount = 0;
  lineThickness = thickness;
}

void ThickLineVertex(Vector2 pos, Color color) {
  if (pointCount < THICK_LINE_MAX_POINTS) {
    pointBuffer[pointCount].pos = pos;
    pointBuffer[pointCount].color = color;
    pointCount++;
  }
}

void ThickLineEnd(bool closed) {
  if (pointCount < 2) {
    return;
  }

  const float halfThick = lineThickness * 0.5f;
  const int n = pointCount;
  const int segCount = closed ? n : (n - 1);

  rlBegin(RL_QUADS);

  for (int i = 0; i < segCount; i++) {
    const int next = (i + 1) % n;
    const Vector2 p0 = pointBuffer[i].pos;
    const Vector2 p1 = pointBuffer[next].pos;
    const Color c0 = pointBuffer[i].color;
    const Color c1 = pointBuffer[next].color;

    const Vector2 dir = Vector2Normalize(Vector2{p1.x - p0.x, p1.y - p0.y});
    const Vector2 perp = Vec2Perp(dir);
    const float ox = perp.x * halfThick;
    const float oy = perp.y * halfThick;

    const Vector2 v0 = Vector2{p0.x + ox, p0.y + oy};
    const Vector2 v1 = Vector2{p0.x - ox, p0.y - oy};
    const Vector2 v2 = Vector2{p1.x - ox, p1.y - oy};
    const Vector2 v3 = Vector2{p1.x + ox, p1.y + oy};

    rlColor4ub(c0.r, c0.g, c0.b, c0.a);
    rlVertex2f(v1.x, v1.y);
    rlVertex2f(v0.x, v0.y);
    rlColor4ub(c1.r, c1.g, c1.b, c1.a);
    rlVertex2f(v3.x, v3.y);
    rlVertex2f(v2.x, v2.y);
  }

  rlEnd();
}
