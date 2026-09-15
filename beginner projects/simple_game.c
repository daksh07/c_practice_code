#include "raylib.h"
#include <stdint.h>
#include <stdio.h>

int main(void) {
  InitWindow(800, 450, "My Game");
  SetTargetFPS(60);
  float ballX = 400;
  float ballY = 225;
  float barX = 400;
  float barY = 430;
  float barWidth = 200;
  float barHeight = 20;
  float ballRadius = 20;
  float ballSpeedX = 5;
  float ballSpeedY = 2;
  uint8_t targetX = 0;
  uint8_t targetY = 0;
  uint8_t targetWidth = 50;
  uint8_t targetHeight = 20;
  int8_t score = 0;
  char scoreBoard [] = "Score: 0";
  while (!WindowShouldClose()) {
    // Movement of the bar
    if (IsKeyDown(KEY_LEFT) && (barX > 0)) {
      barX -= 5;
    }
    if (IsKeyDown(KEY_RIGHT) && (barX + barWidth < 800)) {
      barX += 5;
    }
    
    // Random Movement of the ball and collision detection
    if (ballX - ballRadius <= 0 || ballX + ballRadius >= 800) {
      ballSpeedX *= -1; // reverse horizontal direction
    }
    if (ballY - ballRadius <= 0) {
      ballSpeedY *= -1; // reverse vertical direction
    }
    if ((CheckCollisionCircleRec((Vector2){ballX, ballY}, ballRadius, (Rectangle){barX, barY, barWidth, barHeight}))) {
      ballSpeedY *= -1; // reverse vertical direction
      score++;
      sprintf(scoreBoard, "Score: %d", score);
    }
    if (ballY + ballRadius >= 450){
      ballSpeedY *= -1; // reverse vertical direction
      score--;
      sprintf(scoreBoard, "Score: %d", score);
    }
      ballX += ballSpeedX;
      ballY += ballSpeedY;
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawCircle(ballX, ballY, ballRadius, BLUE);
    DrawRectangle(barX, barY, barWidth, barHeight, RED);
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j <= 800/targetWidth; j++) {
        if (!(CheckCollisionCircleRec((Vector2){ballX, ballY}, ballRadius, (Rectangle){j * targetWidth, i * targetHeight, targetWidth, targetHeight}))) {
          DrawRectangle(j * targetWidth, i * targetHeight, targetWidth, targetHeight, GREEN);
          DrawRectangleLines(j * targetWidth, i * targetHeight, targetWidth, targetHeight, BLACK);
        }
      }
    }
    DrawText(scoreBoard, 200, 450, 20, BLACK);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
