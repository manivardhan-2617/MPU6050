#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// MPU6050 settings
#define MPU_ADDR 0x68
int16_t accX, accY, accZ;
int stepCount = 0;
unsigned long lastStepTime = 0;

// BLE service and characteristic UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"
BLECharacteristic *pCharacteristic;

// Thresholds for step detection
float upperThreshold = 1.2;
float lowerThreshold = 0.8;

void setupMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // Wake up
  Wire.write(0x00);
  Wire.endTransmission();

  // ±2g range
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission();
}

void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  accX = Wire.read() << 8 | Wire.read();
  accY = Wire.read() << 8 | Wire.read();
  accZ = Wire.read() << 8 | Wire.read();
}

void updateStepCount() {
  float ax = accX / 16384.0;
  float ay = accY / 16384.0;
  float az = accZ / 16384.0;
  float accMag = sqrt(ax * ax + ay * ay + az * az);

  unsigned long now = millis();
  if ((accMag > upperThreshold || accMag < lowerThreshold) && (now - lastStepTime > 250)) {
    stepCount++;
    lastStepTime = now;
  }
}

void sendBLEData() {
  String data = "Steps: " + String(stepCount);
  pCharacteristic->setValue(data.c_str());
  pCharacteristic->notify();
  Serial.println(data);
}

void setupBLE() {
  BLEDevice::init("PetMonitor");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  Serial.println("BLE Started");
}

void setup() {
  Serial.begin(115200);
  setupMPU();
  setupBLE();
}

void loop() {
  readMPU();
  updateStepCount();
  sendBLEData();
  delay(1000); // send once every second
}

