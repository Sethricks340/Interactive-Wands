#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);  // Create a sprite buffer

// Ball positions and velocities
int ball1_x = 30, ball1_y = 30, ball1_dx = 5, ball1_dy = 5;
int ball2_x = 80, ball2_y = 50, ball2_dx = 4, ball2_dy = 4;
int ball3_x = 20, ball3_y = 10, ball3_dx = 3, ball3_dy = 3;
int ball4_x = 10, ball4_y = 10, ball4_dx = 2, ball4_dy = 2;
int ball5_x = 10, ball5_y = 10, ball5_dx = 1, ball5_dy = 1;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Turn on backlight

  // Create sprite the same size as the screen
  sprite.createSprite(tft.width(), tft.height());
}

void loop() {
  // Clear sprite each frame
  sprite.fillSprite(TFT_BLACK);

  // Draw balls
  sprite.fillCircle(ball1_x, ball1_y, 2, TFT_RED);
  sprite.fillCircle(ball2_x, ball2_y, 5, TFT_GREEN);
  sprite.fillCircle(ball3_x, ball3_y, 10, TFT_BLUE);
  sprite.fillCircle(ball4_x, ball4_y, 15, TFT_YELLOW);
  sprite.fillCircle(ball5_x, ball5_y, 20, TFT_PURPLE);

  // Push sprite to the screen
  sprite.pushSprite(0, 0);

  // Update positions
  ball1_x += ball1_dx;
  ball1_y += ball1_dy;
  ball2_x += ball2_dx;
  ball2_y += ball2_dy;
  ball3_x += ball3_dx;
  ball3_y += ball3_dy;
  ball4_x += ball4_dx;
  ball4_y += ball4_dy;
  ball5_x += ball5_dx;
  ball5_y += ball5_dy;

  // Bounce off walls
  if (ball1_x <= 10 || ball1_x >= tft.width() - 10) ball1_dx = -ball1_dx;
  if (ball1_y <= 10 || ball1_y >= tft.height() - 10) ball1_dy = -ball1_dy;

  if (ball2_x <= 10 || ball2_x >= tft.width() - 10) ball2_dx = -ball2_dx;
  if (ball2_y <= 10 || ball2_y >= tft.height() - 10) ball2_dy = -ball2_dy;

  if (ball3_x <= 10 || ball3_x >= tft.width() - 10) ball3_dx = -ball3_dx;
  if (ball3_y <= 10 || ball3_y >= tft.height() - 10) ball3_dy = -ball3_dy;

  if (ball4_x <= 10 || ball4_x >= tft.width() - 10) ball4_dx = -ball4_dx;
  if (ball4_y <= 10 || ball4_y >= tft.height() - 10) ball4_dy = -ball4_dy;

  if (ball5_x <= 10 || ball5_x >= tft.width() - 10) ball5_dx = -ball5_dx;
  if (ball5_y <= 10 || ball5_y >= tft.height() - 10) ball5_dy = -ball5_dy;

  delay(5); // Adjust speed
}
