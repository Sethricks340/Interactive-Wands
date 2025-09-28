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

  draw_heart(0,110); //bottom left corner
  draw_stunned(100,50); // center
  draw_shield(150, 50); // right center
}

void draw_stunned(int x_offset, int y_offset){
  tft.fillTriangle(24 + x_offset,8 + y_offset, 12 + x_offset,36 + y_offset,20 + x_offset,36 + y_offset,TFT_YELLOW);
  tft.fillTriangle(20 + x_offset,28 + y_offset,28 + x_offset,28 + y_offset,16 + x_offset,56 + y_offset,TFT_YELLOW);
}

void draw_shield(int x_offset, int y_offset){
  tft.fillTriangle(8 + x_offset,16 + y_offset,20 + x_offset,8 + y_offset,32 + x_offset,16 + y_offset,TFT_BLUE);
  tft.fillTriangle(8 + x_offset,16 + y_offset,32 + x_offset,16 + y_offset,20 + x_offset,32 + y_offset,TFT_BLUE);

  tft.fillTriangle(12 + x_offset,16 + y_offset,20 + x_offset,12 + y_offset,28 + x_offset,16 + y_offset,TFT_DARKGREY);
  tft.fillTriangle(12 + x_offset,16 + y_offset,28 + x_offset,16 + y_offset,20 + x_offset,28 + y_offset,TFT_DARKGREY);
}

void draw_heart(int x_offset, int y_offset){
  tft.fillCircle(10 + x_offset, 10 + y_offset, 6, TFT_RED);
  tft.fillCircle(22 + x_offset, 10 + y_offset, 6, TFT_RED);
  tft.fillTriangle(4 + x_offset, 10 + y_offset, 28 + x_offset, 10 + y_offset, 16 + x_offset, 22 + y_offset, TFT_RED);
}

void loop() {
}