// Git Repo: C:\Users\sethr\OneDrive\Desktop\Interactive Wands

#include <Wire.h>
#include <Arduino.h>
#include <math.h>
#include <TimerOne.h>

#define BLUE 3
#define GREEN 5
#define RED 6

int redValue;
int greenValue;
int blueValue;

const int MAX_SIZE = 4;  // maximum number of strings
String myList[MAX_SIZE]; // array to hold strings
volatile int listCount = 0;       // how many strings are actually in the list (volatile because ISR may set it)

// Spell definitions
String Expelliarmus[2]        = {"YL", "PF"};
String Protego[2]             = {"YR", "PB"};
String Wingardium_leviosa[2]  = {"YR", "PF"};
String Patrificus_totalus[2]  = {"RCW", "YL"};
String Unbreakable_vow[3]     = {"YR", "YL", "PB"};
String Protego_maxima[2]      = {"RCCW", "PB"};
String Sectumsempra[2]        = {"PF", "PB"};
String Incendio[2]            = {"PF", "RCW"};
String Lumos[1]               = {"PF"};
String Nox[1]                 = {"PB"};
String Special_Spell[2]       = {"PB", "PF"};

// MPU6500 I2C address
#define MPU_ADDR 0x68

// MPU registers
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H  0x43

// Scale factors
const float ACCEL_LSB_PER_G = 4096.0;  // ±8g
const float GYRO_LSB_PER_DPS = 16.4;   // ±2000 dps

// Sampling
const float dt = 0.01; // 100 Hz -> 10ms

// Gyro biases
float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;

// Complementary filter coefficient (unused here, left for later)
const float alpha = 0.98;

// gesture flags (volatile because ISR or main may clear them)
volatile bool RCW = false;  // Roll Clockwise
volatile bool RCCW = false; // Roll Counter-Clockwise
volatile bool PF = false;   // Pitch Forward
volatile bool PB = false;   // Pitch Back
volatile bool YR = false;   // Yaw Right
volatile bool YL = false;   // Yaw Left

volatile bool doClearRequest = false; // set by timer ISR -> handled in loop

// --- Helper prototypes (explicit)
bool matchesSequence(const String list[], int listSize, const String seq[], int seqSize);
void addString(const String &s);
void clearFlags();            // clears listCount and flags
void setupMPU();
void calibrateGyro(int samples = 500);

// --- Check all spells (use myList & listCount)
void CheckSpells() {
  // copy listCount atomically into local
  noInterrupts();
  int curSize = listCount;
  interrupts();

  if (curSize == 0) return;

  if (matchesSequence(myList, curSize, Expelliarmus, 2)) {
    Serial.println("Expelliarmus!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Protego, 2)) {
    Serial.println("Protego!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Wingardium_leviosa, 2)) {
    Serial.println("Wingardium Leviosa!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Patrificus_totalus, 2)) {
    Serial.println("Petrificus Totalus!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Unbreakable_vow, 3)) {
    Serial.println("Unbreakable Vow!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Protego_maxima, 2)) {
    Serial.println("Protego Maxima!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Sectumsempra, 2)) {
    Serial.println("Sectumsempra!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Incendio, 2)) {
    Serial.println("Incendio!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Lumos, 1)) {
    Serial.println("Lumos!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Nox, 1)) {
    Serial.println("Nox!");
    clearFlags();
    return;
  }
  if (matchesSequence(myList, curSize, Special_Spell, 2)) {
    Serial.println("Special Spell!");
    clearFlags();
    return;
  }

  // if no matches, you might want to limit list size or drop old entries — up to you
}

// Compare two sequences
bool matchesSequence(const String list[], int listSize, const String seq[], int seqSize) {
  if (listSize != seqSize) return false;
  for (int i = 0; i < listSize; i++) {
    if (list[i] != seq[i]) return false;
  }
  return true;
}

// Add string to list (protect critical section)
void addString(const String &s) {
  noInterrupts();
  if (listCount < MAX_SIZE) {
    myList[listCount++] = s;
  }
  interrupts();
}

// clear list and flags (call from main context)
void clearFlags() {
  noInterrupts();
  listCount = 0;
  RCW = false;
  RCCW = false;
  PF = false;
  PB = false;
  YR = false;
  YL = false;
  interrupts();
}

// write/read helper functions for MPU (same as yours)
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

void calibrateGyro(int samples) {
  Serial.println("Calibrating gyro, keep sensor still...");
  long sumX = 0, sumY = 0, sumZ = 0;
  uint8_t buf[6];
  for (int i = 0; i < samples; i++) {
    readRegisters(GYRO_XOUT_H, 6, buf);
    sumX += toInt16(buf[0], buf[1]);
    sumY += toInt16(buf[2], buf[3]);
    sumZ += toInt16(buf[4], buf[5]);
    delay(5);
  }
  gyroBiasX = sumX / (float)samples;
  gyroBiasY = sumY / (float)samples;
  gyroBiasZ = sumZ / (float)samples;
  Serial.println("Gyro calibration done.");
}

void setupMPU() {
  writeRegister(PWR_MGMT_1, 0x00);   // wake up sensor
  delay(50);
  writeRegister(0x1A, 3);            // DLPF ~44 Hz
  writeRegister(0x1B, 3 << 3);       // gyro ±2000 dps
  writeRegister(0x1C, 2 << 3);       // accel ±8g
  delay(50);
}

// Timer ISR: very small — only set a request flag
void timerIsr() {
  doClearRequest = true;
}

void setup() {
  Serial.begin(115200);

  Timer1.initialize(500000);      // 500000 microseconds = 0.5 second
  Timer1.attachInterrupt(timerIsr);

  Wire.begin();
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  // Default color
  analogWrite(RED, 255);
  analogWrite(GREEN, 255);
  analogWrite(BLUE, 255);

  setupMPU();
  calibrateGyro();
}

void loop() {
  // If ISR requested a clear, do it here (safe context)
  if (doClearRequest) {
    noInterrupts();
    doClearRequest = false;
    interrupts();
    clearFlags();
  }

  static unsigned long lastTime = micros();
  unsigned long now = micros();
  float elapsed = (now - lastTime) / 1000000.0;
  if (elapsed < dt) return;
  lastTime = now;

  uint8_t buf[14];
  if (!readRegisters(ACCEL_XOUT_H, 14, buf)) return;

  // Raw gyro (deg/s)
  float gx = (toInt16(buf[8], buf[9]) - gyroBiasX) / GYRO_LSB_PER_DPS;
  float gy = (toInt16(buf[10], buf[11]) - gyroBiasY) / GYRO_LSB_PER_DPS;
  float gz = (toInt16(buf[12], buf[13]) - gyroBiasZ) / GYRO_LSB_PER_DPS;

  // sensible threshold (degrees per second)
  const float gyroThreshold = 150.0; // tune this by testing

  // Pitch (gx)
  if (gx >= gyroThreshold) {
    if (!PB) { PB = true; addString("PB"); }
  } else if (gx <= -gyroThreshold) {
    if (!PF) { PF = true; addString("PF"); }
  }

  // Roll (gy)
  if (gy >= gyroThreshold) {
    if (!RCW) { RCW = true; addString("RCW"); }
  } else if (gy <= -gyroThreshold) {
    if (!RCCW) { RCCW = true; addString("RCCW"); }
  }

  // Yaw (gz)
  if (gz >= gyroThreshold) {
    if (!YL) { YL = true; addString("YL"); }
  } else if (gz <= -gyroThreshold) {
    if (!YR) { YR = true; addString("YR"); }
  }

  // Check the spellbook
  CheckSpells();
}
