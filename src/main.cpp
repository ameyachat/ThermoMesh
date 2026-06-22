#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define LED_PIN 2

// WiFi credentials
const char* ssid = "ameya";
const char* password = "ameya123";

// MQTT broker
const char* mqtt_server = "172.20.10.2";
const int mqtt_port = 1883;
const char* mqtt_topic = "thermomesh/node1";

Adafruit_BME280 bme;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

SemaphoreHandle_t dataMutex;
float temperature, humidity, pressure;

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ThermoMeshNode1")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

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

void mqttTask(void *pvParameters) {
  while (1) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    float t = temperature;
    float h = humidity;
    float p = pressure;
    xSemaphoreGive(dataMutex);

    char payload[100];
    snprintf(payload, sizeof(payload), 
      "{\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}", 
      t, h, p);
    
    mqttClient.publish(mqtt_topic, payload);
    Serial.println(payload);

    vTaskDelay(pdMS_TO_TICKS(5000));
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

  connectWiFi();
  
  mqttClient.setServer(mqtt_server, mqtt_port);

  dataMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(sensorTask, "SensorTask", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(mqttTask, "MQTTTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(ledTask, "LedTask", 1024, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelete(NULL);
}