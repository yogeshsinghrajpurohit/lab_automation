#include "wifi_manager.h"
#include "mqtt_manager.h"
#include <Preferences.h>
#include <ArduinoJson.h>   // Benoît Blanchon library (keep this one)

// ===== WiFi credentials =====
const char* ssid = "Innovation_Lab";
const char* password = "Mlp0Zaq1";

// ===== MQTT Broker details =====
const char* mqtt_server = "192.168.10.41";
const int mqtt_port = 1883;
const char* mqtt_user = "engineer";
const char* mqtt_password = "123456";
const char* mqtt_topic = "dev/data";
const char* status_topic = "dev/status";

// ===== Initialize managers =====
WiFiManager wifiManager(ssid, password);
MQTTManager mqttManager(mqtt_server, mqtt_port, mqtt_user, mqtt_password, mqtt_topic);
Preferences prefs;

// ===== Define pins =====
int outputPins[] = {23, 22, 21, 19, 18, 17, 16, 4};  // Relays 1-8
int inputPins[]  = {36, 39, 34, 35, 32, 33, 25, 26}; // Inputs 1-8
int totalPins = 8;

// ===== State tracking =====
bool currentStates[8];
bool lastInputStates[8];
unsigned long blockMqttUntil = 0;

// ===== Toggle protection for relay #8 =====
const int TOGGLE_THRESHOLD      = 5;          // 5 toggles
const unsigned long TOGGLE_WINDOW_MS = 5000;  // 5 s window
const unsigned long DEBOUNCE_MS = 50;         // ignore <50 ms noise
const unsigned long BLOCK_MS    = 1800000UL;  // 30 min block

int toggleCount = 0;
unsigned long firstToggleMillis = 0;
unsigned long lastToggleEventMillis = 0;

// ===== Publish all states as JSON =====
void publishAllStates() {
    JsonDocument doc;  // ArduinoJson v7 syntax
    for (int i = 0; i < totalPins; i++) {
        doc[String(i + 1)] = currentStates[i] ? "on" : "off";
    }
    String json;
    serializeJson(doc, json);
    mqttManager.publish(status_topic, json.c_str());
    Serial.printf("📤 Published states to %s: %s\n", status_topic, json.c_str());
}

// ===== MQTT message callback =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (millis() < blockMqttUntil) {
        Serial.println("❌ MQTT blocked - ignoring command");
        return;
    }

    String message;
    for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
    Serial.printf("\n📩 Message: %s\n", message.c_str());

    for (int i = 1; i <= totalPins; i++) {
        String onCmd  = String(i) + "on";
        String offCmd = String(i) + "off";
        if (message == onCmd) {
            currentStates[i - 1] = HIGH;
            digitalWrite(outputPins[i - 1], HIGH);
            prefs.putBool(("relay" + String(i - 1)).c_str(), HIGH);
            Serial.printf("➡ Pin %d HIGH (MQTT)\n", outputPins[i - 1]);
            publishAllStates();
        } else if (message == offCmd) {
            currentStates[i - 1] = LOW;
            digitalWrite(outputPins[i - 1], LOW);
            prefs.putBool(("relay" + String(i - 1)).c_str(), LOW);
            Serial.printf("➡ Pin %d LOW (MQTT)\n", outputPins[i - 1]);
            publishAllStates();
        }
    }
    Serial.println("-------------------------");
}

// ===== Setup =====
void setup() {
    Serial.begin(115200);
    delay(100);

    for (int i = 0; i < totalPins; i++) {
        pinMode(outputPins[i], OUTPUT);
        pinMode(inputPins[i], INPUT); // external pull-downs needed on 36, 39, 34, 35
    }

    prefs.begin("relay_states");
    for (int i = 0; i < totalPins; i++) {
        currentStates[i] = prefs.getBool(("relay" + String(i)).c_str(), LOW);
        digitalWrite(outputPins[i], currentStates[i]);
        Serial.printf("Restored relay %d to %s\n", i + 1, currentStates[i] ? "ON" : "OFF");
    }

    for (int i = 0; i < totalPins; i++) {
        lastInputStates[i] = digitalRead(inputPins[i]);
        if (lastInputStates[i] != currentStates[i]) {
            currentStates[i] = lastInputStates[i];
            digitalWrite(outputPins[i], currentStates[i]);
            prefs.putBool(("relay" + String(i)).c_str(), currentStates[i]);
            Serial.printf("Overrode relay %d to match input (post-outage)\n", i + 1);
        }
    }

    wifiManager.begin();
    mqttManager.setCallback(mqttCallback);
    mqttManager.begin();
}

// ===== Loop =====
void loop() {
    mqttManager.loop();

    static bool initialPublish = false;
    if (!initialPublish && mqttManager.isConnected()) {
        initialPublish = true;
        publishAllStates();
    }

    unsigned long now = millis();
    static unsigned long lastPoll = 0;
    if (now - lastPoll > 100) {
        lastPoll = now;
        for (int i = 0; i < totalPins; i++) {
            bool currInput = digitalRead(inputPins[i]);
            if (currInput != lastInputStates[i]) {
                lastInputStates[i] = currInput;
                currentStates[i] = currInput;
                digitalWrite(outputPins[i], currInput);
                prefs.putBool(("relay" + String(i)).c_str(), currInput);
                Serial.printf("➡ Pin %d %s (Input)\n", outputPins[i],
                              currInput ? "HIGH" : "LOW");
                publishAllStates();

                // === Protection: relay #8 (index 7) toggle window ===
                if (i == 7) {
                    // Debounce
                    if (now - lastToggleEventMillis >= DEBOUNCE_MS) {
                        lastToggleEventMillis = now;

                        if (firstToggleMillis == 0 ||
                            (now - firstToggleMillis) > TOGGLE_WINDOW_MS) {
                            firstToggleMillis = now;
                            toggleCount = 1;
                        } else {
                            toggleCount++;
                        }

                        if (toggleCount >= TOGGLE_THRESHOLD) {
                            blockMqttUntil = now + BLOCK_MS;
                            Serial.printf("🚫 MQTT blocked for %lu ms "
                                          "due to relay 8 toggles (count=%d)\n",
                                          BLOCK_MS, toggleCount);
                            firstToggleMillis = 0;
                            toggleCount = 0;
                        } else {
                            Serial.printf("ℹ Relay 8 toggled (%d/%d) within window\n",
                                          toggleCount, TOGGLE_THRESHOLD);
                        }
                    }
                }
            }
        }
    }
}
