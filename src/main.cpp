#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280 sensor!");
    while (1);
  }
  Serial.println("BME280 found!");
}

void loop() {
  Serial.print("Temperature: ");
  Serial.print(bme.readTemperature());
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");
  
  Serial.print("Pressure: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");
  
  Serial.println("---");
  delay(2000);
}