#include "loading_screen.h"

void DrawLoadingFrame(float progress) {
  const int screenWidth = GetScreenWidth();
  const int screenHeight = GetScreenHeight();

  BeginDrawing();
  ClearBackground(BLACK);

  const int barWidth = screenWidth / 2;
  const int barHeight = 6;
  const int barX = (screenWidth - barWidth) / 2;
  const int barY = screenHeight / 2;

  const Color trackColor = {40, 40, 40, 255};
  const Color fillColor = {0, 230, 242, 255};

  DrawRectangle(barX, barY, barWidth, barHeight, trackColor);
  DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, fillColor);

  EndDrawing();
}
