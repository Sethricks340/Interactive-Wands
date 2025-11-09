//Combined version of sender and reciever, based on random nerd tutorials

/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
*/

#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);  // Create a sprite buffer

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to send data
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));

  // Clear previous text
  sprite.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);

  // Print received data on TFT sprite
  sprite.setTextSize(2);
  sprite.setTextColor(TFT_WHITE);
  sprite.setCursor(0, 0);

  sprite.printf("Message Recieved: %s\n", myData.a);

  sprite.pushSprite(0, 0); // Push to the screen
}

// callback when data is sent (new IDF v5.x signature)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  // Create sprite same size as screen
  sprite.createSprite(tft.width(), tft.height());
  
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Sprite text setup
  sprite.setTextSize(3);
  sprite.setTextColor(TFT_WHITE);
  sprite.setTextDatum(MC_DATUM);

  // Initial display
  sprite.drawString("Module Ready", 120, 67);
  sprite.pushSprite(0, 0);
  delay(10);
}

void loop() {

  if (Serial.available() > 0) {
    String received = Serial.readStringUntil('\n');  // read from Pi
    strcpy(myData.a, received.c_str());
    // delay(10000);

    // Clear sprite
    sprite.fillSprite(TFT_BLACK);

    // Display what we are about to send
    String msg = "Sending:\n";
    msg += "a: " + String(myData.a) + "\n";

    // Print received data on TFT sprite
    sprite.setTextSize(2);
    sprite.setTextColor(TFT_WHITE);
    sprite.setCursor(0, 0);

    sprite.printf("Message Sent: %s\n", myData.a);
    sprite.pushSprite(0, 0); // Push to the screen

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
    }

    delay(100);
  }
}