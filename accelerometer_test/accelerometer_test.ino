//Git Repo: C:\Users\sethr\OneDrive\Desktop\Interactive Wands

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

// Complementary filter coefficient
const float alpha = 0.98;

volatile bool RCW = false;  // Roll Clockwise
volatile bool RCCW = false; // Roll Counter-Clockwise
volatile bool PF = false;   // Pitch Forward
volatile bool PB = false;   // Pitch Back
volatile bool YR = false;   // Yaw Right
volatile bool YL = false;   // Yaw Left

void clearFlags(){
  RCW = false;  // Roll Clockwise
  RCCW = false; // Roll Counter-Clockwise
  PF = false;   // Pitch Forward
  PB = false;   // Pitch Back
  YR = false;   // Yaw Right
  YL = false;   // Yaw Left
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

void calibrateGyro(int samples = 500) {
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

void timerIsr() {
  clearFlags();
}

void setup() {
  Serial.begin(115200);
  Serial.begin(115200);        
  Timer1.initialize(1000000);      // 0.5 second
  Timer1.attachInterrupt(timerIsr);
  Wire.begin();
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
  delay(100);

  redValue = 255; // choose a value between 1 and 255 to change the color.
  greenValue = 255;
  blueValue = 255;

  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);
  
  setupMPU();
  calibrateGyro();

}

void loop() {

  static unsigned long lastTime = micros();
  unsigned long now = micros();
  float elapsed = (now - lastTime) / 1000000.0;
  if (elapsed < dt) return;
  lastTime = now;

  uint8_t buf[14];
  if (!readRegisters(ACCEL_XOUT_H, 14, buf)) return;

  // Raw gyro
  float gx = (toInt16(buf[8], buf[9]) - gyroBiasX) / GYRO_LSB_PER_DPS;
  float gy = (toInt16(buf[10], buf[11]) - gyroBiasY) / GYRO_LSB_PER_DPS;
  float gz = (toInt16(buf[12], buf[13]) - gyroBiasZ) / GYRO_LSB_PER_DPS;

  // Acceleration in 'Pitch'
  //Wand tip towards head
  if (gx >= 1000){
    PB = true;
  }
  //Wand tip away from head
  else if (gx <= -1000){
    PF = true;
  }

  // Acceleration in 'Roll'
  // Roll CW, looking down from hand
  if (gy >= 1000){
    RCW = true;
  }
  //Roll CCW, looking down from hand
  else if (gy <= -1000){
    RCCW = true;
  }

  // Acceleration in 'Yaw'
  // Flick to the left
  if (gz >= 1000){
    YL = true;
  }
  else if (gz <= -1000){
  // Flick to the right
    YR = true;
  }

  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);

}
