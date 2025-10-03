// Track Joystick or buttons for menu selection

/* Once option is selected, send out message 
     with ESP_NOW every 10 seconds with the time remaining for late arrivals.
     (wands will filter if they don't need it) */

// handleTimer()

/* Once time is over, return to main menu. No other action needed. 
    (Wands keep track of time on their own)*/
// reset somehow (does setup all over again)

#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite arrowSprite = TFT_eSprite(&tft);  // Sprite instance
int arrow_y_offset = 0;                       //0, 25, 50, 75

// BROADCAST TO ALL ESPs
uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

esp_now_peer_info_t peerInfo;

int totalSeconds = 60;

const int buttonPin = 33;  // the pin your button is connected to

unsigned long lastPress = 0;
const int debounceDelay = 200;  // ms

unsigned long buttonPressStart = 0;
bool buttonHeld = false;

const unsigned long holdTime = 2000;  // 2 seconds to start the game

bool gameStarted = false;

int last_sec = 0;

int last_ESP_send = 1000;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  pinMode(buttonPin, INPUT_PULLUP);

  arrowSprite.createSprite(40, 135);

  move_arrow(0);

  arrowSprite.pushSprite(0, 20);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Show_menu();
}

void loop() {
  if (gameStarted) {
    int curr_sec = millis() / 1000;
    // only update once per second
    if (curr_sec != last_sec) {
      if (last_ESP_send - totalSeconds >= 10) {
        last_ESP_send = totalSeconds;
        Send_ESP_Now_String("base:" + String(totalSeconds));
      }
      handleTimer();
      last_sec = curr_sec;
    }
    return;
  }

  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {  // button is pressed
    if (!buttonHeld) {       // first time detecting press
      buttonPressStart = millis();
      buttonHeld = true;
    }

    // check if held long enough to start game
    if (millis() - buttonPressStart >= holdTime) {
      start_game();
      buttonHeld = false;  // prevent multiple triggers
    }

  } else {  // button released
    if (buttonHeld) {
      // it was a short click
      if (millis() - buttonPressStart < holdTime) {
        toggle_option();  // move arrow
      }
      buttonHeld = false;  // reset
    }
  }
}

void start_game() {
  gameStarted = true;
  Send_ESP_Now_String("base:" + String(totalSeconds));
}

void Send_ESP_Now_String(String text) {
  char message[32];
  text.trim();
  text.toCharArray(message, sizeof(message));
  esp_now_send(broadcastAddress, (uint8_t*)message, strlen(message));
}

void Show_menu() {
  draw_text("Options: ", 0, 0, 2);
  draw_text("1 Minute Game", 40, 25, 2);
  draw_text("3 Minute Game", 40, 50, 2);
  draw_text("5 Minute Game", 40, 75, 2);
  draw_text("10 Minute Game", 40, 100, 2);
}

void handleTimer() {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  if (totalSeconds <= 0) {
    ESP.restart();
  }
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);
  tft.setTextSize(4);
  tft.fillScreen(TFT_BLACK);  // fills the entire screen with black
  int16_t x = (tft.width() - tft.textWidth(String(buffer))) / 2;
  int16_t y = 57;
  tft.drawString(String(buffer), x, y);

  totalSeconds--;
}

void draw_text(String text, int x_offset, int y_offset, int text_size) {
  tft.setTextSize(text_size);
  tft.drawString(text, x_offset, y_offset);
}

void move_arrow(int y_offset) {
  arrowSprite.fillSprite(TFT_BLACK);
  arrowSprite.fillTriangle(
    25, 12 + y_offset,  // Tip on the right
    5, 2 + y_offset,    // Top left
    5, 22 + y_offset,   // Bottom left
    TFT_YELLOW);
  arrowSprite.pushSprite(0, 20);
}

void toggle_option() {
  arrow_y_offset += 25;
  if (arrow_y_offset > 75) arrow_y_offset = 0;
  move_arrow(arrow_y_offset);
  seconds_from_menu(arrow_y_offset);
}

void seconds_from_menu(int arrow_offset) {
  if (arrow_offset == 0) {
    totalSeconds = 60;
  } else if (arrow_offset == 25) {
    totalSeconds = 180;
  } else if (arrow_offset == 50) {
    totalSeconds = 300;
  } else if (arrow_offset == 75) {
    totalSeconds = 600;
  }
}