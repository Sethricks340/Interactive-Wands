#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// BROADCAST TO ALL ESPs
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

esp_now_peer_info_t peerInfo;

int totalSeconds;

int timer = 0;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  // Serial.begin(115200); 

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
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  Show_menu();

}

void loop() {

  // Track Joystick or buttons for menu selection

  /* Once option is selected, send out message 
     with ESP_NOW every 10 seconds with the time remaining for late arrivals.
     (wands will filter if they don't need it) */

  // printTimeLeft()
  // Send_ESP_Now_String("base:totalSeconds")

  /* Once time is over, return to main menu. No other action needed. 
    (Wands keep track of time on their own)*/
  // Show_menu();

}

void Send_ESP_Now_String(String text){
  char message[32];
  text.trim();
  text.toCharArray(message, sizeof(message));
  esp_now_send(broadcastAddress, (uint8_t*)message, strlen(message));
}

void Show_menu(){
  // options: 
      // 1 min (60s)
      // 3 min (180s)
      // 5 min (300s)
  // continue
}

void printTimeLeft() {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);

  // Add TFT logic
}




