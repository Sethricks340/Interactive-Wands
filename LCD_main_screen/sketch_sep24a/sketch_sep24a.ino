#include <TFT_eSPI.h>
#include <SPI.h>

// 240×135 pixels
TFT_eSPI tft = TFT_eSPI();

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  // Create heart sprite
  TFT_eSprite heart = TFT_eSprite(&tft);
  heart.createSprite(32, 32);      // size of the sprite
  heart.fillSprite(TFT_BLACK);     // clear to black

  uint16_t heartColor = TFT_RED;
  heart.fillCircle(10, 10, 6, heartColor);     // top-left lobe
  heart.fillCircle(22, 10, 6, heartColor);     // top-right lobe
  heart.fillTriangle(4, 10, 28, 10, 16, 28, heartColor);  // bottom triangle

  int heartWidth = 32;
  int heartHeight = 32;
  int bottomY = tft.height() - heartHeight;

  for (int i = 0; i < 5; i++) {
      int xPos = i * heartWidth;
      heart.pushSprite(xPos, bottomY);
  }
}

void loop() {
  // nothing here yet
}
