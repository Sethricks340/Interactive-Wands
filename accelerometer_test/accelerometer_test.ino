//Git Repo: "C:\Users\sethr\OneDrive\Desktop\Interactive Wands"

#include <Wire.h>
#include <Arduino.h>
#include <math.h>

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
const float GYRO_LSB_PER_DPS = 16.4;   // ±2000 dps

// Sampling
const float dt = 0.01; // 100 Hz -> 10ms

volatile bool RCW = false;  // Roll Clockwise
volatile bool RCCW = false; // Roll Counter-Clockwise
volatile bool PF = false;   // Pitch Forward
volatile bool PB = false;   // Pitch Back
volatile bool YR = false;   // Yaw Right
volatile bool YL = false;   // Yaw Left

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

void setup() {
  Serial.begin(115200);        
  Wire.begin();
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  digitalWrite(RED, HIGH);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
  delay(100);

  redValue = 0; // choose a value between 1 and 255 to change the color.
  greenValue = 0;
  blueValue = 0;

  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);
  
  setupMPU();
  Serial.println("READY");
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
  float gx = (toInt16(buf[8], buf[9])) / GYRO_LSB_PER_DPS;
  float gy = (toInt16(buf[10], buf[11])) / GYRO_LSB_PER_DPS;
  float gz = (toInt16(buf[12], buf[13])) / GYRO_LSB_PER_DPS;

  // Acceleration in 'Pitch'
  //Wand tip towards head
  if (gx >= 1000){
    PB = true;
    Serial.println("PB");
    Serial.print("gx: "); Serial.print(String(gx, 2)); Serial.print(" | "); Serial.print("gy: "); Serial.print(String(gy, 2)); Serial.print(" | "); Serial.print("gz: "); Serial.print(String(gz, 2)); Serial.println(" | ");
  }
  //Wand tip away from head
  else if (gx <= -1000){
    PF = true;
    Serial.println("PF");
    Serial.print("gx: "); Serial.print(String(gx, 2)); Serial.print(" | "); Serial.print("gy: "); Serial.print(String(gy, 2)); Serial.print(" | "); Serial.print("gz: "); Serial.print(String(gz, 2)); Serial.println(" | ");
  }

  // Acceleration in 'Roll'
  // Roll CW, looking down from hand
  if (gy >= 1000){
    RCW = true;
    Serial.println("RCW");
    Serial.print("gx: "); Serial.print(String(gx, 2)); Serial.print(" | "); Serial.print("gy: "); Serial.print(String(gy, 2)); Serial.print(" | "); Serial.print("gz: "); Serial.print(String(gz, 2)); Serial.println(" | ");
  }
  //Roll CCW, looking down from hand
  else if (gy <= -1000){
    RCCW = true;
    Serial.println("RCCW");
    Serial.print("gx: "); Serial.print(String(gx, 2)); Serial.print(" | "); Serial.print("gy: "); Serial.print(String(gy, 2)); Serial.print(" | "); Serial.print("gz: "); Serial.print(String(gz, 2)); Serial.println(" | ");
  }

  // Acceleration in 'Yaw'
  // Flick to the left
  if (gz >= 1000){
    YL = true;
    Serial.println("YL");
    Serial.print("gx: "); Serial.print(String(gx, 2)); Serial.print(" | "); Serial.print("gy: "); Serial.print(String(gy, 2)); Serial.print(" | "); Serial.print("gz: "); Serial.print(String(gz, 2)); Serial.println(" | ");
  }
  else if (gz <= -1000){
  // Flick to the right
    YR = true;
    Serial.println("YR");
    Serial.print("gx: "); Serial.print(String(gx, 2)); Serial.print(" | "); Serial.print("gy: "); Serial.print(String(gy, 2)); Serial.print(" | "); Serial.print("gz: "); Serial.print(String(gz, 2)); Serial.println(" | ");
  }

  analogWrite(RED, redValue);
  analogWrite(GREEN, greenValue);
  analogWrite(BLUE, blueValue);

}