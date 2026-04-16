/**
 * Embedded Solar Monitoring System
 * --------------------------------
 * Target MCU : ESP32
 * Sensors    : INA219 (V/I, I2C), SSD1306 OLED (I2C)
 * Author     : Ghulam Sarwar
 *
 * Measures solar panel output V, I and computes instantaneous power.
 * Shows live values on a 0.96" OLED and publishes JSON to an MQTT
 * broker over WiFi every 5 seconds.
 *
 * MIT License
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// -------- user config --------
const char* WIFI_SSID   = "YOUR_SSID";
const char* WIFI_PASS   = "YOUR_PASSWORD";
const char* MQTT_HOST   = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC  = "ghulam/solar/telemetry";

#define SDA_PIN 21
#define SCL_PIN 22
#define OLED_W 128
#define OLED_H 64

Adafruit_INA219   ina219;
Adafruit_SSD1306  oled(OLED_W, OLED_H, &Wire, -1);
WiFiClient        wifiClient;
PubSubClient      mqtt(wifiClient);

struct Sample {
    float busV;      // V
    float current;   // mA
    float power;     // mW
    float energyWh;  // cumulative
} sample;

uint32_t lastPubMs = 0;
uint32_t lastSampleMs = 0;

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi");
    uint8_t retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries++ < 40) {
        delay(250);
        Serial.print(".");
    }
    Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " FAIL");
}

void mqttReconnect() {
    while (!mqtt.connected()) {
        String cid = "solar-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        if (mqtt.connect(cid.c_str())) {
            Serial.println("MQTT OK");
        } else {
            delay(1000);
        }
    }
}

void drawOled(const Sample& s) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.println(F("Solar Monitor"));
    oled.drawLine(0, 10, OLED_W, 10, SSD1306_WHITE);

    oled.setTextSize(2);
    oled.setCursor(0, 16);
    oled.printf("%5.2f V", s.busV);
    oled.setCursor(0, 36);
    oled.printf("%5.0f mA", s.current);

    oled.setTextSize(1);
    oled.setCursor(0, 56);
    oled.printf("P:%.2fW  E:%.2fWh", s.power / 1000.0, s.energyWh);
    oled.display();
}

void publishJson(const Sample& s) {
    char buf[160];
    snprintf(buf, sizeof(buf),
        "{\"v\":%.3f,\"i_mA\":%.2f,\"p_mW\":%.2f,\"e_Wh\":%.3f,\"t\":%lu}",
        s.busV, s.current, s.power, s.energyWh, millis());
    mqtt.publish(MQTT_TOPIC, buf);
    Serial.println(buf);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    Wire.begin(SDA_PIN, SCL_PIN);
    if (!ina219.begin()) {
        Serial.println("INA219 not found");
        while (true) delay(1000);
    }
    ina219.setCalibration_16V_400mA();  // adjust to your panel range

    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED not found");
    }

    connectWiFi();
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
}

void loop() {
    if (!mqtt.connected()) mqttReconnect();
    mqtt.loop();

    uint32_t now = millis();
    if (now - lastSampleMs >= 500) {
        float dtH = (now - lastSampleMs) / 3600000.0f;
        lastSampleMs = now;

        sample.busV    = ina219.getBusVoltage_V();
        sample.current = ina219.getCurrent_mA();
        sample.power   = ina219.getPower_mW();
        sample.energyWh += (sample.power / 1000.0f) * dtH;

        drawOled(sample);
    }

    if (now - lastPubMs >= 5000) {
        lastPubMs = now;
        if (mqtt.connected()) publishJson(sample);
    }
}
