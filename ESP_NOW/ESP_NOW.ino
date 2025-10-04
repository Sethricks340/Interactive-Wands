//Git Repo: "C:\Users\sethr\OneDrive\Desktop\Interactive Wands"

// Notes for users:
// * Works best with quick flicks
// * Press button, do spell, release button
// * Works best with a small pause inbetween each movement

// TODO: 
//    Make Buzz non-blocking
//    Create filters for more complicated spell effects?

#include <Wire.h>
#include <math.h>
#include <esp_now.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>

bool game_started = false;

TFT_eSPI tft = TFT_eSPI();
// TFT_eSprite sprite = TFT_eSprite(&tft);  // Create a sprite buffer

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

char message[32];

esp_now_peer_info_t peerInfo;

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
int waiting_timer = 3000; // in ms
unsigned long last_waiting_update = 0;
int last_random = 0;

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
int remaining_game_time = 0;
int LED_on = false;
int LED_start_time = 3;
int LED_timeout = 3000; // 3 seconds (3000ms)
volatile long Points = 0; // Total Points in this game

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

String loading_messages[] = {
  // Spaces added/deleted in strings for custom text wrapping. 
  "Giving Dobby a sock...",
  "Running from Aragog...",
  "Sending an   owl...",
  "Fighting off Dementors...",
  "Stirring a   potion...",
  "Killing a    Basilisk...",          
  "Making an    Unbreakable  Vow...",
  "Eating a     chocolate    frog...",
  "Waiting for  Hogwarts     letter...",
  "Catching the Snitch...",
  "Stealing a   dragon from  Gringotts...",
  "Blowing up anAunt...",         
  "Looking in   the Mirror ofErised...",
  "Searching forHorcruxes...",
  "Playing      Wizard's     Chess...",
};

Spell spells[] = {
// Spell name, length of moves, moves, RGB, self-shield, self-stun, self-life, others-shield, others-stun, others-life, spell owner
  {"Expelliarmus",       2, {"YL", "PF"},    {255, 0, 0},     {0, 3, 0, 0, 7, 0},     "None"},       // Red
  {"Sectumsempra",       2, {"PF", "PB"},    {255, 36, 0},    {0, 3, -50, 3, 0, -100},   "None"},       // Redish-orange
  {"Protego",            2, {"YR", "PB"},    {255, 127, 0},   {13, 3, 25, 0, 0, 0},    "None"},       // Yellow
  {"Protego Maxima",     2, {"RCCW", "PB"},  {0, 0, 255},     {10, 10, 0, 10, 0, 0},  "None"},       // Blue
  {"Wingardium Leviosa", 2, {"YR", "PF"},    {180, 30, 180},  {0, 10, 100, 3, 0, -50},   "None"},       // Darker Pink
  {"Patrificus Totalus", 2, {"RCW", "RCCW"}, {180, 30, 100},  {0, 10, 0, 3, 0, -100},    "None"},       // Lighter Pink
  {"Incendio",           2, {"PF", "RCW"},   {0, 100, 34},    {15, 10, 0, 3, 0, -50}, "None"},       // Teal
};

Spell characterSpells[] = {
  {"Congelare Lacare",   2, {"PB", "PF"},    {102, 153, 0},   {5, 0, -50, 3, 0, -175},   "Molly Weasley"},       // Yellow-green1
  {"Marauder's Map",      2, {"PB", "PF"},    {51, 204, 0},   {15, 3, 25, 0, 0, 0},   "Fred Weasley"},        // Yellow-green2
  {"Alohamora",          2, {"PB", "PF"},    {15, 255, 15},   {0, 3, 50, 0, 0, 0},    "Hermione Granger"},    // Coral green
  {"Advada Kedavera",    2, {"PB", "PF"},    {0, 255, 0},     {0, 3, -200, 0, 0, -200},   "Lord Voldemort"},      // Green
  {"Eat Slugs",          2, {"PB", "PF"},    {45, 255, 45},   {0, 3, -100, 0, 10, -100},  "Ron Weasley"},         // Greenish-blue
  {"Episky",             2, {"PB", "PF"},    {0, 255, 147},   {10, 3, 100, 0, 0, 100},    "Luna Lovegood"},       // Sky blue
  {"Invisibility Cloak", 2, {"PB", "PF"},    {255, 255, 255}, {15, 10, 100, 0, 0, 0},   "Harry Potter"},        // White
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
String ESP_message = "";
bool ESP_recv = false;

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(true);

  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);  // Backlight on

  Serial.begin(115200);    

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);    
  
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

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register recieve callback
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

  control_LED(0, 0, 0); // LED off
  
  setupMPU();
  getCharacterSpell();
}

void loop() {
  // If not enough time has passed (less than dt), skip this iteration
  static unsigned long lastTime = micros();
  unsigned long now = micros();
  float elapsed = (now - lastTime) / 1000000.0;
  if (elapsed < dt) return;

  if (ESP_recv) in_loop_ESP_recv();

  if (!game_started) {
    if (millis() - last_waiting_update >= waiting_timer) {
      draw_random_waiting_message(); 
      last_waiting_update = millis();  // reset timer
    }
    return;
  }

  listening = !digitalRead(buttonPin); // Low (false) means button pressed

  spell_recognizing_sequence();

  if (millis() - LED_start_time > LED_timeout){
    control_LED(0, 0, 0);
  }

  lastTime = now;

  check_movements();
  check_timers();
}

//--- ESP-NOW Functions ---//

// callback function that will be executed when data is received
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len){
  if (ESP_recv) return;

  // If you want the MAC address:
  const uint8_t *mac = info->src_addr;   // <- new way to get sender MAC

  // Copy data to your buffer
  memcpy(message, incomingData, len);
  message[len] = '\0'; // null terminate

  ESP_message = String(message);
  ESP_recv = true;
}

void in_loop_ESP_recv(){
  // If receiving a message from the base station for the first time
  if (ESP_message.startsWith("base:") && !game_started) {
    game_started = true;
    start_sequence();
  }

  // Spell has no effect if your shield is on
  if (shield){
    return;
  }

  for (int i = 0; i < NUM_SPELLS; i++) {
    if (strcmp(spells[i].name, ESP_message.c_str()) == 0) {
      draw_message_box_first_row("Hit by:");
      draw_message_box_second_row(ESP_message);
      buzzVibrator(250, 2);
      if (spells[i].effects[3]) handle_self_shield(spells[i].effects[3]);
      if (spells[i].effects[4]) handle_self_stun(spells[i].effects[4]);
      if (spells[i].effects[5]) handle_self_points(spells[i].effects[5]);
      return;
    }
  }

  for (int i = 0; i < NUM_CHARACTER_SPELLS; i++) {
    if (strcmp(characterSpells[i].name, ESP_message.c_str()) == 0) {
      draw_message_box_first_row("Hit by:");
      draw_message_box_second_row(ESP_message);
      buzzVibrator(250, 2);
      if (characterSpells[i].effects[3]) handle_self_shield(characterSpells[i].effects[3]);
      if (characterSpells[i].effects[4]) handle_self_stun(characterSpells[i].effects[4]);
      if (characterSpells[i].effects[5]) handle_self_points(characterSpells[i].effects[5]);
      return;
    }
  }
  ESP_recv = false;
  ESP_message = "";
}

//--- MPU Functions ---//

void setupMPU() {
  writeRegister(PWR_MGMT_1, 0x00);   // wake up sensor
  delay(50);
  writeRegister(0x1A, 3);            // DLPF ~44 Hz
  writeRegister(0x1B, 3 << 3);       // gyro ±2000 dps
  writeRegister(0x1C, 2 << 3);       // accel ±8g
  delay(50);
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

//--- Spell Functions ---//

void spell_recognizing_sequence(){
  if (stunned){
    clearSpellChecker();
    return;
  }
  if (listCount && !listening){ // Movement detected while button was pressed, and button is now released
    Spell spell = checkThroughSpells();
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

void clearSpellChecker(){
  listCount = 0;       
}

void addToSpellChecker(String spell){
  spellChecker[listCount] = spell;
  listCount++;
}

Spell checkThroughSpells() {
  Spell result;
  result.name = "None"; // default

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
      result = spells[i];
      return result;
    }
  }
  // --- check character spell explicitly ---
  if (listCount == 2 && spellChecker[0] == "PB" && spellChecker[1] == "PF") {
    result = characterSpell;
    return result;
  }
  return result; // default "None"
}

void doSpell(Spell spell){
  if (last_spell == spell.name){
    draw_message_box_first_row("Can't repeat spell");
    draw_message_box_second_row(" ");
    buzzVibrator(750, 1);
    return;
  }
  last_spell = spell.name;
  draw_message_box_first_row(spell.name);

  control_LED(spell.colors[0], spell.colors[1], spell.colors[2]);

  buzzVibrator(250, 2);

  if (spell.effects[0]) handle_self_shield(spell.effects[0]);
  if (spell.effects[1]) handle_self_stun(spell.effects[1]);
  if (spell.effects[2]) handle_self_points(spell.effects[2]);
  
  String spell_to_send = spell.name;
  spell_to_send.trim();
  spell_to_send.toCharArray(message, sizeof(message));
  // Send spell to any nearby wands using ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t*)message, strlen(message));
}


//--- Effects Functions ---//

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

    // --- Handle shield timer --- //
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

    // --- Handle shield timer --- //
    if (remaining_game_time > 0) {
      draw_game_timer(String(handleTimer(remaining_game_time)));
      remaining_game_time--;
    } 
    else if (remaining_game_time <= 0){
      end_sequence();
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

void handle_self_points(int amount){
  Points += amount;
  update_points_print();
}

void control_LED(int redValue, int greenValue, int blueValue){
  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);
}


//--- Drawing Functions ---//

void draw_text(String text, int x_offset, int y_offset, int text_size){
  tft.setTextSize(text_size);
  tft.drawString(text, x_offset, y_offset);
}

void clear_screen(){
  tft.fillScreen(TFT_BLACK); // fills the entire screen with black
}

void start_sequence(){
  // Do this all the first time to start the game
  tft.fillScreen(TFT_GREEN);
  int16_t x = (tft.width() - tft.textWidth("START!")) / 2;
  int16_t y = 57;
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setTextSize(3);
  tft.drawString("START!", x, y);
  buzzVibrator(250, 2);

  clear_screen();
  tft.setTextColor(TFT_WHITE); 
  tft.setTextSize(2);
  // Message in form base:number
  String after = String(ESP_message).substring(5);
  remaining_game_time = after.toInt();
  draw_heart_icon();
  update_points_print();
  draw_name(self_name);
  draw_clock_icon();
}

void end_sequence(){
  // This sequence happens when the game timer runs out
  tft.fillScreen(TFT_RED);
  tft.setTextSize(3);
  int16_t x = (tft.width() - tft.textWidth("GAME OVER!")) / 2;
  int16_t y = 57;
  tft.setTextColor(TFT_BLACK, TFT_RED);
  tft.drawString("GAME OVER!", x, y);
  buzzVibrator(250, 2);
  print_score_screen();
}

void print_score_screen(){
  clear_screen();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(3);
  draw_name(self_name);
  draw_message_box_first_row("Points: " + String(Points));
  draw_message_box_second_row("(Press to Continue)");      

  listening = !digitalRead(buttonPin); // Low (false) means button pressed
  while(!listening){
    listening = !digitalRead(buttonPin);
  }
  ESP.restart();
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

void draw_heart_icon() {
  tft.fillCircle(10, 98, 6, TFT_RED);  
  tft.fillCircle(22, 98, 6, TFT_RED);  
  tft.fillTriangle(4, 98, 28, 98, 16, 110, TFT_RED);  
}

void update_points_print() {
  // Clear the game timer
  tft.fillRect(35, 93, 240, 15, TFT_BLACK);

  // Rewrite game timer
  draw_text(String(Points), 35, 93, 2);   
}

void draw_clock_icon() {
  tft.fillCircle(16, 124, 10, TFT_DARKGREY);
  tft.fillCircle(16, 124, 8, TFT_WHITE);
  tft.fillRect(16, 124, 8, 2, TFT_DARKGREY);
  tft.fillRect(16, 117, 2, 7, TFT_DARKGREY);
}

void draw_game_timer(String time){
  tft.fillRect(35,120,70,15,TFT_BLACK);
  draw_text(time, 35, 120, 2);
}

String handleTimer(int time) {
  int minutes = time / 60;
  int seconds = time % 60;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);
  return String(buffer);
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

void draw_random_waiting_message(){
  int random_number = random(0, sizeof(loading_messages) / sizeof(loading_messages[0]));
  while (random_number == last_random) return;
  last_random = random_number;
  clear_screen();
  tft.setCursor(0, 47);     
  tft.setTextSize(3);       
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.println(loading_messages[random_number]);        
}