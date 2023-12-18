#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

char ssid[] = "TP-Link_6B02";
char pass[] = "79381222";

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Callback");
  Serial.println((char) payload[0]);
}

const char* mqttServer = "test.mosquitto.org";
WiFiClient wifiClient = WiFiClient();
PubSubClient mqttClient(mqttServer, 1883, callback, wifiClient);

void reconnect() {
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected()) {
    Serial.println("Reconnecting to MQTT Broker..");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected.");
      // subscribe to topic
    }
  }
}

#define REPORTING_PERIOD_MS 700
PulseOximeter pox;
uint32_t tsLastReport = 0;

void onBeatDetected() {
  Serial.println("Beat!");
}

const int oneWireBus = D6;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);

  Serial.print("Initializing pulse oximeter..");
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);
  sensors.begin();
}

void loop() {
  pox.update();
  float oxi = pox.getSpO2();
  float rate = pox.getHeartRate();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("Heart rate:");
    Serial.print(rate);
    Serial.print("bpm / SpO2:");
    Serial.print(oxi);
    Serial.println("%");

    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);
    Serial.print(temperatureC);
    Serial.println("ºC");

    tsLastReport = millis();
    send_to_server(String(rate) + " bpm", "ESP8266_nhiptim");
    send_to_server(String(oxi) + " %", "ESP8266_Oxi");
    send_to_server(String(temperatureC) + ("ºC"), "ESP8266_nhietdo");
  }
}

void send_to_server(String data, const char* topic) {
  if (!mqttClient.connected()) {
    reconnect();
    mqttClient.loop();
  }
  const char* new_data = data.c_str();
  mqttClient.publish(topic, new_data);
}
