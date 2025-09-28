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

  for (int i = 0; i < 4; i++) {
    draw_heart(i * 27, 110);    //bottom left corner
  }
  // draw_stunned(100, 105); // center
  // draw_stunned_timer("35", 120, 120);
  draw_stunned(120, 105); // center
  draw_text("35", 140, 120, 2);

  draw_shield(170, 100); // right center
  draw_text("53", 210, 120, 2);

  draw_text("Hermione Granger", 0, 0, 2);

  draw_text("Can't do same spell", 0, 47, 2);
  draw_text("twice in a row", 0, 67, 2);
}

void draw_text(String name, int x_offset, int y_offset, int text_size){
  tft.setTextSize(text_size);
  tft.drawString(name, x_offset, y_offset);
}

void draw_stunned(int x_offset, int y_offset){
  tft.fillTriangle(12 + x_offset, 4 + y_offset, 6 + x_offset, 18 + y_offset, 10 + x_offset, 18 + y_offset, TFT_YELLOW);
  tft.fillTriangle(10 + x_offset, 14 + y_offset, 14 + x_offset, 14 + y_offset, 8 + x_offset, 28 + y_offset, TFT_YELLOW);
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