#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// MAX30102
MAX30105 particleSensor;
#define MAX_BUFFER_SIZE 32
uint32_t irBuffer[MAX_BUFFER_SIZE];
uint32_t redBuffer[MAX_BUFFER_SIZE];
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// DS18B20
#define ONE_WIRE_BUS D3  // GPIO0
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temperatureC = 0.0;

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  if (particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    particleSensor.setup(60, 4, 2, 100, 411, 4096);
    particleSensor.setPulseAmplitudeRed(0x3F);
    particleSensor.setPulseAmplitudeIR(0x3F);
    Serial.println("MAX30102 Ready");
  } else {
    lcd.clear();
    lcd.print("MAX30102 Error");
    while (1);
  }

  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    lcd.clear();
    lcd.print("DS18B20 Error");
    while (1);
  }

  delay(2000);
  lcd.clear();
}

void loop() {
  // Read Temperature
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (temp != DEVICE_DISCONNECTED_C) {
    temperatureC = temp;
  }

  // Read HR and SpO2
  for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
    while (!particleSensor.available()) particleSensor.check();
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
    delay(2);
  }

  maxim_heart_rate_and_oxygen_saturation(irBuffer, MAX_BUFFER_SIZE, redBuffer,
    &spo2, &validSPO2, &heartRate, &validHeartRate);

  if (validHeartRate && heartRate > 0) {
    heartRate = constrain(heartRate, 60, 120);
  }

  // Show on LCD
  lcd.setCursor(0, 0);
  lcd.print("HR:");
  validHeartRate ? lcd.print(heartRate) : lcd.print("--");
  lcd.print(" SpO2:");
  validSPO2 ? lcd.print(spo2) : lcd.print("--");

  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(temperatureC, 1);
  lcd.print((char)223); lcd.print("C");

  // Also print to Serial Monitor
  Serial.print("HR: ");
  Serial.print(validHeartRate ? heartRate : 0);
  Serial.print(" | SpO2: ");
  Serial.print(validSPO2 ? spo2 : 0);
  Serial.print(" | Temp: ");
  Serial.println(temperatureC);

  delay(2000);
}
