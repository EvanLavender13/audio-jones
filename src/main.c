#include "raylib.h"

int main(void)
{
    InitWindow(800, 600, "AudioJones");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(BLACK);
            DrawText("AudioJones", 320, 280, 20, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
