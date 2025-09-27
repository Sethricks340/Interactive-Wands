//Git Repo: "C:\Users\sethr\OneDrive\Desktop\Interactive Wands"

// Notes for users:
// * Works best with quick flicks
// * Press button, do spell, release button
// * Works best with a small pause inbetween each movement

//TODO: Add shielding logic
//TODO: Add countdown prints to shield and disable
//TODO: Add dummy ISR

#include <Wire.h>
#include <Arduino.h>
#include <math.h>

const int buttonPin = 33; // the pin your button is connected to
int buttonState = 0;

#define RED 25
#define GREEN 26
#define BLUE 27

// MPU6500 I2C address
#define MPU_ADDR 0x68

// MPU registers
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H  0x43

// Scale factors
const float GYRO_LSB_PER_DPS = 16.4;   // ±2000 dps

// Sampling
const float dt = 0.01; // 100 Hz -> 10ms

volatile bool RCW = false;  // Roll Clockwise
volatile bool RCCW = false; // Roll Counter-Clockwise
volatile bool PF = false;   // Pitch Forward
volatile bool PB = false;   // Pitch Back
volatile bool YR = false;   // Yaw Right
volatile bool YL = false;   // Yaw Left

// TODO: Are these valid?
// Possible flags for wand
volatile bool shielded = false;  // Wand can't be affected by spells
volatile bool disabled = false;  // Wand can't cast spells
volatile bool listening = false; // If the button is pressed

volatile unsigned long disableTime = 0; // Time wand is disabled for
volatile unsigned long shieldTime = 0; // Time wand is shielded for
volatile int lives = 3; // Total lives, 3 is max

const int threshold = 800;

// Spell definitions
struct Spell {
    const char* name;
    int length;       // number of movements
    const char* moves[4];  // max movements per spell
    int colors[3];
    int affects[6];
    String wizard_name;
};

struct SpellResults {
    const char* name;
    int red;
    int green;
    int blue;

    int self_shield;
    int self_disable;
    int self_life;

    int others_shield;
    int others_disable;
    int others_life;
};

Spell spells[] = {
  {"Expelliarmus",       2, {"YL", "PF"},    {255, 0, 0},     {0, 3, 0, 3, 7, 0},     "None"},       // Red
  {"Sectumsempra",       2, {"PF", "PB"},    {255, 36, 0},    {0, 3, -1, 3, 0, -1},   "None"},       // Redish-orange
  {"Protego",            2, {"YR", "PB"},    {255, 127, 0},   {13, 3, 0, 3, 0, 0},    "None"},       // Yellow
  {"Protego_maxima",     2, {"RCCW", "PB"},  {0, 0, 255},     {10, 10, 0, 10, 0, 0},  "None"},       // Blue
  {"Wingardium_leviosa", 2, {"YR", "PF"},    {180, 30, 180},  {0, 10, 1, 3, 0, -1},   "None"},       // Darker Pink
  {"Patrificus_totalus", 2, {"RCW", "RCCW"}, {180, 30, 100},  {0, 5, 0, 3, 0, -1},    "None"},       // Lighter Pink
  {"Incendio",           2, {"PF", "RCW"},   {0, 100, 34},    {15, 15, -1, 3, 0, -1}, "None"},       // Teal
};

Spell characterSpells[] = {
  {"Congelare_lacare",   2, {"PB", "PF"},    {102, 153, 0},   {5, 0, -1, 3, 0, -1},   "Molly"},      // Yellow-green1
  {"Marauders_map",      2, {"PB", "PF"},    {51, 204, 0},    {30, 10, 1, 0, 0, 0},   "Fred"},       // Yellow-green2
  {"Alohamora",          2, {"PB", "PF"},    {15, 255, 15},   {0, 3, 1, -1, 0, 0},    "Hermione"},   // Coral green
  {"Advada_kedavera",    2, {"PB", "PF"},    {0, 255, 0},     {0, 3, -2, 3, 0, -3},   "Voldemort"},  // Green
  {"Eat_slugs",          2, {"PB", "PF"},    {45, 255, 45},   {0, 3, -1, 3, 10, -1},  "Ron"},        // Greenish-blue
  {"Episky",             2, {"PB", "PF"},    {0, 255, 147},   {10, 3, 1, 0, 0, 1},    "Luna"},       // Sky blue
  {"Invisibility_cloak", 2, {"PB", "PF"},    {255, 255, 255}, {30, 10, 1, 0, 0, 0},   "Harry"},      // White
};

const int NUM_SPELLS = sizeof(spells) / sizeof(spells[0]);
const int NUM_CHARACTER_SPELLS = sizeof(characterSpells) / sizeof(characterSpells[0]);

const int MAX_SIZE = 4; 
String spellChecker[MAX_SIZE]; 
volatile int listCount = 0;    

const String self_name = "Molly";
// const String self_name = "Fred";
// const String self_name = "Hermione";
// const String self_name = "Voldemort";
// const String self_name = "Ron";
// const String self_name = "Luna";
// const String self_name = "Harry";
Spell characterSpell = {"_", 0, {"PB", "PF"}, {0, 0, 0}, {0, 0, 0, 0, 0, 0}, "_"};

String last_spell = "";

int motorPin = 14;

void setup() {
  Serial.begin(115200);        
  Wire.begin();
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  delay(100);

  control_LED(0, 0, 0); // LED off
  
  setupMPU();
  getCharacterSpell();
  Serial.print(characterSpell.wizard_name);
  Serial.println(": READY");
}

void loop() {

  //TODO: remove debugging logic
  // if (!digitalRead(buttonPin)){
  //   control_LED(0, 255, 0); // Green

  // }
  // else{
  //   control_LED(0, 0, 0); // LED off
  // }
  // return;



  // If not enough time has passed (less than dt), skip this iteration
  static unsigned long lastTime = micros();
  unsigned long now = micros();
  float elapsed = (now - lastTime) / 1000000.0;
  if (elapsed < dt) return;

  listening = !digitalRead(buttonPin); // Low (false) means button pressed
  
  // Check global bools for filtering data sampling  
  if ((now - lastTime) >= disableTime && disabled){
    Serial.println("Wand re-enabeled");
    control_LED(0, 0, 0); // LED off
    buzzVibrator(250, 2);
    disabled = false;
  }
  if (disabled){
    // Serial.println("Disabled is true, returning...");
    return;
  }
  if (listCount && !listening){ // No movement for 1 second, and button is released
    SpellResults spell = checkThroughSpells();
    if (spell.name != "None"){
      doSpell(spell);
    }
    else{
      Serial.println("Spell not recognized");
    }
    clearSpellChecker();
  }

  lastTime = now;

  // Create a buffer to hold 14 bytes from the MPU6500:
  uint8_t buf[14];
  // Read 14 sequential registers starting at ACCEL_XOUT_H (0x3B)
  if (!readRegisters(ACCEL_XOUT_H, 14, buf)) return;

  // Raw gyro
  float gx = (toInt16(buf[8], buf[9])) / GYRO_LSB_PER_DPS;
  float gy = (toInt16(buf[10], buf[11])) / GYRO_LSB_PER_DPS;
  float gz = (toInt16(buf[12], buf[13])) / GYRO_LSB_PER_DPS;

  // Acceleration in 'Pitch'
  // Wand tip towards head
  if (gx >= threshold){
    if (!PB && listening){
      PB = true;
      addToSpellChecker("PB");
    }
  }
  else {
    PB = false;
  }
  
  // Wand tip away from head
  if (gx <= -threshold){
    if (!PF && listening){
      PF = true;
      addToSpellChecker("PF");
    }
  }
  else {
    PF = false;
  }

  // Acceleration in 'Roll'
  // Roll CW, looking down from hand
  if (gy >= threshold){
    if (!RCW && listening){
      RCW = true;
      addToSpellChecker("RCW");
    }
  }
  else{
    RCW = false;
  }

  // Roll CCW, looking down from hand
  if (gy <= -threshold){
    if (!RCCW && listening){
      RCCW = true;
      addToSpellChecker("RCCW");
    }
  }
  else{
    RCCW = false;
  }

  // Acceleration in 'Yaw'
  // Flick to the left
  if (gz >= threshold){
    if (!YL && listening){
      YL = true;
      addToSpellChecker("YL");
    }
  }
  else{
    YL = false;
  }
  
  // Flick to the right
  if (gz <= -threshold){
    if (!YR && listening){
      YR = true;
      addToSpellChecker("YR");
    }
  }
  else{
    YR = false;
  }
}

void getCharacterSpell(){
  for (int i = 0; i < NUM_CHARACTER_SPELLS; i++) {
    if (self_name == characterSpells[i].wizard_name){
      characterSpell.name = characterSpells[i].name;
      characterSpell.length = characterSpells[i].length;
      for(int j = 0; j < 4; j++) characterSpell.moves[j] = characterSpells[i].moves[j];
      for(int j = 0; j < 3; j++) characterSpell.colors[j] = characterSpells[i].colors[j];
      for(int j = 0; j < 6; j++) characterSpell.affects[j] = characterSpells[i].affects[j];
      characterSpell.wizard_name = characterSpells[i].wizard_name;
    }
  }
}

void buzzVibrator(int duration, int times){
  for (int i = 0; i < times; i++) {
    // Turn motor ON
    digitalWrite(motorPin, HIGH);
    delay(duration);  // keep on for 1 second

    // Turn motor OFF
    digitalWrite(motorPin, LOW);
    delay(duration);  // keep off for 1 second
  }
}

void clearSpellChecker(){
  listCount = 0;       
}

void addToSpellChecker(String spell){
  spellChecker[listCount] = spell;
  listCount++;
}

void printSpellChecker() {
  Serial.print("Movements: ");
  for (int i = 0; i < listCount; i++) {
    Serial.print(spellChecker[i]); Serial.print(" ");
  }
  Serial.println();
}

SpellResults checkThroughSpells() {
  SpellResults result;
  result.name = "None"; // default
  result.red = result.green = result.blue = 0;
  result.self_shield = result.self_disable = result.self_life =
  result.others_shield = result.others_disable = result.others_life = 0;

  // --- check normal spells ---
  for (int i = 0; i < NUM_SPELLS; i++) {
    if (listCount != spells[i].length) continue;

    bool equals = true;
    for (int j = 0; j < spells[i].length; j++) {
      if (spellChecker[j] != spells[i].moves[j]) {
        equals = false;
        break;
      }
    }

    if (equals) {
      result.name = spells[i].name;
      result.red   = spells[i].colors[0];
      result.green = spells[i].colors[1];
      result.blue  = spells[i].colors[2];

      result.self_shield   = spells[i].affects[0];
      result.self_disable  = spells[i].affects[1];
      result.self_life     = spells[i].affects[2];
      result.others_shield = spells[i].affects[3];
      result.others_disable= spells[i].affects[4];
      result.others_life   = spells[i].affects[5];

      return result;
    }
}
  // --- check character spell explicitly ---
  // (example: 2 moves PB and PF)
  if (listCount == 2 && spellChecker[0] == "PB" && spellChecker[1] == "PF") {
    result.name = characterSpell.name;
    result.red   = characterSpell.colors[0];
    result.green = characterSpell.colors[1];
    result.blue  = characterSpell.colors[2];

    result.self_shield   = characterSpell.affects[0];
    result.self_disable  = characterSpell.affects[1];
    result.self_life     = characterSpell.affects[2];
    result.others_shield = characterSpell.affects[3];
    result.others_disable= characterSpell.affects[4];
    result.others_life   = characterSpell.affects[5];

    return result;
  }
  return result; // default "None"
}

void doSpell(SpellResults spell){
  if (last_spell == spell.name){
    Serial.println("Can't do same spell twice in a row.");
    return;
  }

  // Print spell name
  Serial.println(spell.name);
  last_spell = spell.name;

  // Control LED
  control_LED(spell.red, spell.green, spell.blue);

  buzzVibrator(250, 2);

  // Handle self shield
  shielded = (spell.self_shield > 0);
  shieldTime = (unsigned long)spell.self_shield * 1000000UL; // Convert seconds to micros
  if(shielded){
    Serial.print("Self shielded for ");
    Serial.print(shieldTime / 1000000UL);
    Serial.println(" seconds");
  } else {
    Serial.println("Self not shielded");
  }

  // Handle self disable
  disableTime = (unsigned long)spell.self_disable * 1000000UL; // Convert seconds to micros
  disabled = (spell.self_disable > 0);
  if (disabled){
    Serial.print("Self disabled for ");
    Serial.print(disableTime / 1000000UL);
    Serial.println(" seconds");
  } else {
    Serial.println("Self not disabled");
  }

  // Update lives with clamping
  lives += spell.self_life;
  if (lives < 0) lives = 0;
  if (lives > 5) lives = 5;
  Serial.print("Lives: ");
  Serial.println(lives);

  Serial.print("others_shield: ");
  Serial.println(spell.others_shield);
  Serial.print("others_disable: ");
  Serial.println(spell.others_disable);
  Serial.print("others_life: ");
  Serial.println(spell.others_life);
  Serial.println(" ");
    // result.self_shield = result.self_disable = result.self_life = result.others_shield = result.others_disable = result.others_life = 0;
}

void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool readRegisters(uint8_t reg, uint8_t count, uint8_t *dest) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(MPU_ADDR, count);
  for (uint8_t i = 0; i < count && Wire.available(); i++) {
    dest[i] = Wire.read();
  }
  return true;
}

int16_t toInt16(uint8_t hi, uint8_t lo) {
  return (int16_t)((hi << 8) | lo);
}

void setupMPU() {
  writeRegister(PWR_MGMT_1, 0x00);   // wake up sensor
  delay(50);
  writeRegister(0x1A, 3);            // DLPF ~44 Hz
  writeRegister(0x1B, 3 << 3);       // gyro ±2000 dps
  writeRegister(0x1C, 2 << 3);       // accel ±8g
  delay(50);
}

void control_LED(int redValue, int greenValue, int blueValue){
  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);
}











































// //Combined version of sender and reciever, based on random nerd tutorials

// /*
//   Rui Santos & Sara Santos - Random Nerd Tutorials
//   Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
//   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
// */

// #include <esp_now.h>
// #include <WiFi.h>
// #include <TFT_eSPI.h>
// #include <SPI.h>

// TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite sprite = TFT_eSprite(&tft);  // Create a sprite buffer

// // REPLACE WITH YOUR RECEIVER MAC Address
// uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// // Structure example to send data
// typedef struct struct_message {
//   char a[32];
//   int b;
//   float c;
//   bool d;
// } struct_message;

// // Create a struct_message called myData
// struct_message myData;

// esp_now_peer_info_t peerInfo;

// // callback function that will be executed when data is received
// void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
//   memcpy(&myData, incomingData, sizeof(myData));

//   // Clear previous text
//   sprite.fillRect(0, 0, tft.width(), tft.height(), TFT_BLACK);

//   // Print received data on TFT sprite
//   sprite.setTextSize(2);
//   sprite.setTextColor(TFT_WHITE);
//   sprite.setCursor(0, 0);

//   sprite.printf("Message Recieved: %s\n", myData.a);

//   sprite.pushSprite(0, 0); // Push to the screen
// }

// // callback when data is sent (new IDF v5.x signature)
// void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
//   Serial.print("\r\nLast Packet Send Status:\t");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

// void setup() {
//   tft.init();
//   tft.setRotation(1);
//   tft.fillScreen(TFT_BLACK);

//   pinMode(32, OUTPUT);
//   digitalWrite(32, HIGH);  // Backlight on

//   // Create sprite same size as screen
//   sprite.createSprite(tft.width(), tft.height());
  
//   // Init Serial Monitor
//   Serial.begin(115200);
 
//   // Set device as a Wi-Fi Station
//   WiFi.mode(WIFI_STA);

//   // Init ESP-NOW
//   if (esp_now_init() != ESP_OK) {
//     Serial.println("Error initializing ESP-NOW");
//     return;
//   }

//   // Register send callback
//   esp_now_register_send_cb(OnDataSent);
//   esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  
//   // Register peer
//   memcpy(peerInfo.peer_addr, broadcastAddress, 6);
//   peerInfo.channel = 0;  
//   peerInfo.encrypt = false;
  
//   // Add peer        
//   if (esp_now_add_peer(&peerInfo) != ESP_OK){
//     Serial.println("Failed to add peer");
//     return;
//   }

//   // Sprite text setup
//   sprite.setTextSize(3);
//   sprite.setTextColor(TFT_WHITE);
//   sprite.setTextDatum(MC_DATUM);

//   // Initial display
//   sprite.drawString("Module Ready", 120, 67);
//   sprite.pushSprite(0, 0);
//   delay(10);
// }

// void loop() {

//   if (Serial.available() > 0) {
//     String received = Serial.readStringUntil('\n');  // read from Pi
//     strcpy(myData.a, received.c_str());

//     // Clear sprite
//     sprite.fillSprite(TFT_BLACK);

//     // Display what we are about to send
//     String msg = "Sending:\n";
//     msg += "a: " + String(myData.a) + "\n";

//     // Print received data on TFT sprite
//     sprite.setTextSize(2);
//     sprite.setTextColor(TFT_WHITE);
//     sprite.setCursor(0, 0);

//     sprite.printf("Message Sent: %s\n", myData.a);
//     sprite.pushSprite(0, 0); // Push to the screen

//     // Send message via ESP-NOW
//     esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
//     if (result == ESP_OK) {
//       Serial.println("Sent with success");
//     } else {
//       Serial.println("Error sending the data");
//     }

//     delay(100);
//   }
// }
