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

  draw_heart_row(4);

  draw_stunned();

  draw_shield(); 

  draw_name("Hermione Granger");

  // Examples of long messages
  // draw_message_box_first_row("Can't repeat spell");
  draw_message_box_first_row("Wingardium Leviosa");
  draw_message_box_second_row("Invisibility Cloak");
}

void draw_text(String text, int x_offset, int y_offset, int text_size){
  tft.setTextSize(text_size);
  tft.drawString(text, x_offset, y_offset);
}

void draw_message_box_first_row(String text){
  tft.fillRect(0, 47, tft.width(), 20, TFT_BLACK);
  draw_text(text, 0, 47, 2);
}

void draw_message_box_second_row(String text){
  tft.fillRect(0, 67, tft.width(), 20, TFT_BLACK);
  draw_text(text, 0, 67, 2);
}

void draw_name(String name){
  draw_text(name, 0, 0, 2);
}

void draw_stunned(){
  tft.fillTriangle(132, 109, 126, 123, 130, 123, TFT_YELLOW);
  tft.fillTriangle(130, 119, 134, 119, 128, 133, TFT_YELLOW);
}

void draw_shield(){
  tft.fillTriangle(178,116,190,108,202,116,TFT_BLUE);
  tft.fillTriangle(178,116,202,116,190,132,TFT_BLUE);

  tft.fillTriangle(182,116,190,112,198,116,TFT_DARKGREY);
  tft.fillTriangle(182,116,198,116,190,128,TFT_DARKGREY);
}

void write_shield_timer(String time){
  // Clear the timer area
  tft.fillRect(210,120,25,25,TFT_BLACK);

  // Rewrite timer
  draw_text(time, 210, 120, 2);
}

void draw_heart_row(int amount) {
  //Clear current heart row
  tft.fillRect(0,105,120,120,TFT_BLACK);

  if (amount > 4) amount = 4;

  for (int i = 0; i < amount; i++) {
    int x = i * 27;
    tft.fillCircle(10 + x, 120, 6, TFT_RED);
    tft.fillCircle(22 + x, 120, 6, TFT_RED);
    tft.fillTriangle(4 + x, 120, 28 + x, 120, 16 + x, 132, TFT_RED);
  }
}

void write_stunned_timer(String time){
  // Clear the timer area
  tft.fillRect(140,120,25,25,TFT_BLACK);

  // Rewrite timer
  draw_text(time, 140, 120, 2); 
}

void clear_stunned_area(){
  // Clear the timer area
  tft.fillRect(140,120,25,25,TFT_BLACK);

  // Clear stunned icon area
  tft.fillRect(120,105,30,30,TFT_BLACK);
}

void clear_shield_area(){
  // Clear the timer area
  tft.fillRect(210,120,25,25,TFT_BLACK);

  // Clear stunned icon area
  tft.fillRect(178, 108, 25, 25, TFT_BLACK);
}

int last_sec = 0;
int remaining_stun_time = 0;
int remaining_shield_time = 0;
bool stunned = false;
bool shield = false;

void check_timers() {
  // current second since boot
  int curr_sec = millis() / 1000;

  // only update once per second
  if (curr_sec != last_sec) {
    last_sec = curr_sec;

    // --- Handle stunned timer ---
    if (remaining_stun_time > 0) {
      if (stunned) {
        write_stunned_timer(String(remaining_stun_time));
        remaining_stun_time--;
      }
    } else {
      if (stunned) draw_message_box_first_row("You're un-stunned!");
      stunned = false;
      clear_stunned_area();
    }

    // --- Handle shield timer ---
    if (remaining_shield_time > 0) {
      if (shield) {
        write_shield_timer(String(remaining_shield_time));
        remaining_shield_time--;
      }
    } else {
      if (shield) draw_message_box_second_row("Shield disabled!");
      shield = false;
      clear_shield_area();
    }
  }
}

void loop() {
  if (stunned || shield){
    check_timers();
  }
}