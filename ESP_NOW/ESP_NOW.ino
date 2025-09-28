//Git Repo: "C:\Users\sethr\OneDrive\Desktop\Interactive Wands"

// Notes for users:
// * Works best with quick flicks
// * Press button, do spell, release button
// * Works best with a small pause inbetween each movement

#include <Wire.h>
#include <math.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite sprite = TFT_eSprite(&tft);  // Create a sprite buffer

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

volatile bool shield = false;  // Wand can't be affected by spells
volatile bool stunned = false;  // Wand can't cast spells
volatile bool listening = false; // If the button is pressed

int last_sec = 0;
int remaining_stun_time = 0;
int remaining_shield_time = 0;
int LED_on = false;
int LED_start_time = 3;
int LED_timeout = 3000; // 3 seconds (3000ms)
volatile int lives = 3; // Total lives, 4 is max

const int threshold = 800;

// Spell definitions
struct Spell {
    const char* name;
    int length;       // number of movements
    const char* moves[4];  // max movements per spell
    int colors[3];
    int effects[6];
    String wizard_name;
};

struct SpellResults {
    const char* name;
    int red;
    int green;
    int blue;

    int self_shield;
    int self_stun;
    int self_life;

    int others_shield;
    int others_stun;
    int others_life;
};

Spell spells[] = {
// Spell name, length of moves, moves, RGB, self-shield, self-stun, self-life, others-shield, others-stun, others-life, spell owner
  {"Expelliarmus",       2, {"YL", "PF"},    {255, 0, 0},     {0, 3, 0, 3, 7, 0},     "None"},       // Red
  {"Sectumsempra",       2, {"PF", "PB"},    {255, 36, 0},    {0, 3, -1, 3, 0, -1},   "None"},       // Redish-orange
  {"Protego",            2, {"YR", "PB"},    {255, 127, 0},   {13, 3, 0, 3, 0, 0},    "None"},       // Yellow
  {"Protego Maxima",     2, {"RCCW", "PB"},  {0, 0, 255},     {10, 10, 0, 10, 0, 0},  "None"},       // Blue
  {"Wingardium Leviosa", 2, {"YR", "PF"},    {180, 30, 180},  {0, 10, 1, 3, 0, -1},   "None"},       // Darker Pink
  {"Patrificus Totalus", 2, {"RCW", "RCCW"}, {180, 30, 100},  {0, 5, 0, 3, 0, -1},    "None"},       // Lighter Pink
  {"Incendio",           2, {"PF", "RCW"},   {0, 100, 34},    {15, 15, -1, 3, 0, -1}, "None"},       // Teal
};

Spell characterSpells[] = {
  {"Congelare Lacare",   2, {"PB", "PF"},    {102, 153, 0},   {5, 0, -1, 3, 0, -1},   "Molly Weasley"},      // Yellow-green1
  {"Marauder's Map",      2, {"PB", "PF"},    {51, 204, 0},    {30, 10, 1, 0, 0, 0},   "Fred Weasley"},       // Yellow-green2
  {"Alohamora",          2, {"PB", "PF"},    {15, 255, 15},   {0, 3, 1, -1, 0, 0},    "Hermione Granger"},   // Coral green
  {"Advada Kedavera",    2, {"PB", "PF"},    {0, 255, 0},     {0, 3, -2, 3, 0, -3},   "Lord Voldemort"},  // Green
  {"Eat Slugs",          2, {"PB", "PF"},    {45, 255, 45},   {0, 3, -1, 3, 10, -1},  "Ron Weasley"},        // Greenish-blue
  {"Episky",             2, {"PB", "PF"},    {0, 255, 147},   {10, 3, 1, 0, 0, 1},    "Luna Lovegood"},       // Sky blue
  {"Invisibility Cloak", 2, {"PB", "PF"},    {255, 255, 255}, {30, 10, 1, 0, 0, 0},   "Harry Potter"},      // White
};

const int NUM_SPELLS = sizeof(spells) / sizeof(spells[0]);
const int NUM_CHARACTER_SPELLS = sizeof(characterSpells) / sizeof(characterSpells[0]);

const int MAX_SIZE = 4; 
String spellChecker[MAX_SIZE]; 
volatile int listCount = 0;    

const String self_name = "Molly Weasley";
// const String self_name = "Fred Weasley";
// const String self_name = "Hermione Granger";
// const String self_name = "Lord Voldemort";
// const String self_name = "Ron Weasley";
// const String self_name = "Luna Lovegood";
// const String self_name = "Harry Potter";
Spell characterSpell = {"_", 0, {"PB", "PF"}, {0, 0, 0}, {0, 0, 0, 0, 0, 0}, "_"};

String last_spell = "";

int motorPin = 14;

void setup() {

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  // sprite.createSprite(tft.width(), tft.height());

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

  draw_heart_row(3);

  delay(100);

  control_LED(0, 0, 0); // LED off
  
  setupMPU();
  getCharacterSpell();
  draw_name(self_name);
}

void loop() {
  // If not enough time has passed (less than dt), skip this iteration
  static unsigned long lastTime = micros();
  unsigned long now = micros();
  float elapsed = (now - lastTime) / 1000000.0;
  if (elapsed < dt) return;

  listening = !digitalRead(buttonPin); // Low (false) means button pressed

  spell_recognizing_sequence();

  if (millis() - LED_start_time > LED_timeout){
    control_LED(0, 0, 0);
  }

  lastTime = now;

  check_movements();
  if (stunned || shield){
    check_timers();
  }
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
      clear_stunned_area();
      if (stunned) {
        draw_message_box_first_row("You're un-stunned!");
        buzzVibrator(250, 2);
      }
      stunned = false;
    }

    // --- Handle shield timer ---
    if (remaining_shield_time > 0) {
      if (shield) {
        write_shield_timer(String(remaining_shield_time));
        remaining_shield_time--;
      }
    } else {
      clear_shield_area();
      if (shield){
        draw_message_box_second_row("Shield disabled!");
        buzzVibrator(250, 2);
      }
      shield = false;
    }
  }
}

void spell_recognizing_sequence(){
  if (listCount && !listening){ // Movement detected while button was pressed, and button is now released
    SpellResults spell = checkThroughSpells();
    if (spell.name != "None"){
      doSpell(spell);
      // LED timer here
      LED_start_time = millis();
      LED_on = true;
    }
    else{
      draw_message_box_first_row("Spell not recognized");
      draw_message_box_second_row(" ");
      buzzVibrator(750, 1);
    }
    clearSpellChecker();
  }
}

void check_movements(){
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
      for(int j = 0; j < 6; j++) characterSpell.effects[j] = characterSpells[i].effects[j];
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
  result.self_shield = result.self_stun = result.self_life =
  result.others_shield = result.others_stun = result.others_life = 0;

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

      result.self_shield   = spells[i].effects[0];
      result.self_stun  = spells[i].effects[1];
      result.self_life     = spells[i].effects[2];
      result.others_shield = spells[i].effects[3];
      result.others_stun= spells[i].effects[4];
      result.others_life   = spells[i].effects[5];

      return result;
    }
}
  // --- check character spell explicitly ---
  if (listCount == 2 && spellChecker[0] == "PB" && spellChecker[1] == "PF") {
    result.name = characterSpell.name;
    result.red   = characterSpell.colors[0];
    result.green = characterSpell.colors[1];
    result.blue  = characterSpell.colors[2];

    result.self_shield   = characterSpell.effects[0];
    result.self_stun  = characterSpell.effects[1];
    result.self_life     = characterSpell.effects[2];
    result.others_shield = characterSpell.effects[3];
    result.others_stun= characterSpell.effects[4];
    result.others_life   = characterSpell.effects[5];

    return result;
  }
  return result; // default "None"
}

void doSpell(SpellResults spell){
  if (last_spell == spell.name){
    draw_message_box_first_row("Can't repeat spell");
    draw_message_box_second_row(" ");
    buzzVibrator(750, 1);
    return;
  }
  last_spell = spell.name;
  draw_message_box_first_row(spell.name);

  control_LED(spell.red, spell.green, spell.blue);

  buzzVibrator(250, 2);

  if (spell.self_shield) handle_self_shield(spell.self_shield);
  if (spell.self_stun) handle_self_stun(spell.self_stun);
  if (spell.self_life) handle_self_life(spell.self_life);
  
  // TODO: Make others logic 
  // handle_others_shield(spell.others_shield);
  // handle_others_stun(spell.others_stun);
  // handle_others_life(spell.others_life);

}

void handle_self_shield(int time){
  draw_shield(); 
  shield = true;
  remaining_shield_time = time;
}

void handle_self_stun(int time){
  draw_stunned(); 
  stunned = true;
  remaining_stun_time = time;
}

void handle_self_life(int amount){
  lives += amount;
  if (lives > 4){
    lives = 4;
  }
  if (lives <= 0){
    // TODO: Dead logic here
  }
  draw_heart_row(lives);
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
