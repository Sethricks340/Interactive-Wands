//Git Repo: "C:\Users\sethr\OneDrive\Desktop\Interactive Wands"

// Notes for users:
// * Works best with quick flicks
// * Press button, do spell, release button
// * Works best with a small pause inbetween each movement

#include <Wire.h>
#include <Arduino.h>
#include <math.h>

const int buttonPin = 2; // the pin your button is connected to
int buttonState = 0;

#define BLUE 3
#define GREEN 5
#define RED 6

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
    String name;
    int length;       // number of movements
    String moves[4];  // max movements per spell
    int colors[3];
    int affects[6];
};

struct SpellResults {
    String name;
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
    {"Expelliarmus", 2, {"YL", "PF"}, {255, 36, 0}, {0, 3, 0, 3, 7, 0}},
    {"Protego", 2, {"YR", "PB"}, {255, 127, 0}, {13, 3, 0, 3, 0, 0}},
    {"Wingardium_leviosa", 2, {"YR", "PF"}, {180, 30, 180}, {0, 10, 1, 3, 0, -1}},
    {"Patrificus_totalus", 2, {"RCW", "RCCW"}, {255, 0, 255}, {0, 5, 0, 3, 0, -1}},
    {"Protego_maxima", 2, {"RCCW", "PB"}, {0, 0, 255}, {10, 10, 0, 10, 0, 0}},
    {"Sectumsempra", 2, {"PF", "PB"}, {255, 0, 0}, {0, 3, -1, 3, 0, -1}},
    {"Incendio", 2, {"PF", "RCW"}, {255, 50, 0}, {25, 25, -1, 3, 0, -1}},
    {"Lumos", 1, {"PF"}, {255, 255, 255}, {0, 0, 0, 0, 0, 0}},
    {"Nox", 1, {"PB"}, {0, 0, 0}, {0, 0, 0, 0, 0, 0}},
    {"Character_Spell", 2, {"PB", "PF"}, {0, 255, 0}, {0, 0, 0, 0, 0, 0}} //TODO: add special spells per wand
};

const int NUM_SPELLS = sizeof(spells) / sizeof(spells[0]);

const int MAX_SIZE = 4; 
String spellChecker[MAX_SIZE]; 
volatile int listCount = 0;       

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
  delay(100);

  control_LED(0, 0, 0); // LED off
  
  setupMPU();
  Serial.println("READY");
}

void loop() {

  if (Serial.available() > 0){
    String message = Serial.readStringUntil('\n');  // read from Pi

    if (message == "colors"){
      Serial.println("Enter redValue:");
      while (!Serial.available());
      int redValue = Serial.parseInt();
      
      Serial.println("Enter greenValue:");
      while (!Serial.available());
      int greenValue = Serial.parseInt();
      
      Serial.println("Enter blueValue:");
      while (!Serial.available());
      int blueValue = Serial.parseInt();

      control_LED(redValue, greenValue, blueValue); // LED off
    }
  }

  // If not enough time has passed (less than dt), skip this iteration
  static unsigned long lastTime = micros();
  unsigned long now = micros();
  float elapsed = (now - lastTime) / 1000000.0;
  if (elapsed < dt) return;

  listening = !digitalRead(buttonPin); // Low (false) means button pressed
  
  // Check global bools for filtering data sampling  
  if ((now - lastTime) >= disableTime && disabled){
    Serial.println("Wand re-enabeled");
    disabled = false;
    control_LED(0, 0, 0); // LED off
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
    result.self_shield = result.self_disable = result.self_life = result.others_shield = result.others_disable = result.others_life = 0;

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
            result.red = spells[i].colors[0];
            result.green = spells[i].colors[1];
            result.blue = spells[i].colors[2];

            result.self_shield = spells[i].affects[0];
            result.self_disable = spells[i].affects[1];
            result.self_life = spells[i].affects[2];
            result.others_shield = spells[i].affects[3];
            result.others_disable = spells[i].affects[4];
            result.others_life = spells[i].affects[5];

            return result;
        }
    }

    return result;
}

void doSpell(SpellResults spell){
  // Print spell name
  Serial.println(spell.name);

  // Control LED
  control_LED(spell.red, spell.green, spell.blue);

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
  if (lives > 3) lives = 3;
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