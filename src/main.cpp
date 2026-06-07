#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

#define LED_PIN 2

Adafruit_BME280 bme;

SemaphoreHandle_t dataMutex;
float temperature, humidity, pressure;

void sensorTask(void *pvParameters) {
  while (1) {
    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0F;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    temperature = t;
    humidity = h;
    pressure = p;
    xSemaphoreGive(dataMutex);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void printTask(void *pvParameters) {
  while (1) {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    float t = temperature;
    float h = humidity;
    float p = pressure;
    xSemaphoreGive(dataMutex);

    Serial.println("---");
    Serial.printf("Temperature: %.2f C\n", t);
    Serial.printf("Humidity: %.2f %%\n", h);
    Serial.printf("Pressure: %.2f hPa\n", p);

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void ledTask(void *pvParameters) {
  pinMode(LED_PIN, OUTPUT);
  while (1) {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void setup() {
  Serial.begin(115200);

  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found!");
    while (1);
  }

  dataMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(printTask, "PrintTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(ledTask, "LedTask", 1024, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}