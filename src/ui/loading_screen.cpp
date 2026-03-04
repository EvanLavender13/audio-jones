#include "loading_screen.h"

void DrawLoadingFrame(float progress) {
  int screenWidth = GetScreenWidth();
  int screenHeight = GetScreenHeight();

  BeginDrawing();
  ClearBackground(BLACK);

  int barWidth = screenWidth / 2;
  int barHeight = 6;
  int barX = (screenWidth - barWidth) / 2;
  int barY = screenHeight / 2;

  Color trackColor = {40, 40, 40, 255};
  Color fillColor = {0, 230, 242, 255};

  DrawRectangle(barX, barY, barWidth, barHeight, trackColor);
  DrawRectangle(barX, barY, (int)(barWidth * progress), barHeight, fillColor);

  EndDrawing();
}
