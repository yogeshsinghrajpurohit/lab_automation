#include "wifi_manager.h"
#include "mqtt_manager.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// ===== WiFi =====
const char* ssid = "Innovation_Lab";
const char* password = "Mlp0Zaq1";

// ===== MQTT =====
const char* mqtt_server = "192.168.10.41";
const int mqtt_port = 1883;
const char* mqtt_user = "engineer";
const char* mqtt_password = "123456";

const char* status_topic  = "dev/status";
const char* command_topic = "dev/command";

// ===== Objects =====
WiFiManager wifiManager(ssid, password);
MQTTManager mqttManager(mqtt_server, mqtt_port, mqtt_user, mqtt_password);
Preferences prefs;

// ===== Pins =====
int outputPins[] = {23,22,21,19,18,17,16,4};
int inputPins[]  = {36,39,34,35,32,33,25,26};
const int totalPins = 8;

// ===== States =====
bool relayState[8];
bool lastSwitchState[8];
bool switchChangedAfterBoot[8] = {false};

// ===== Publish relay states =====
void publishAllStates() {
    Serial.println("[STATE] Publishing relay states");
    JsonDocument doc;
    for (int i = 0; i < totalPins; i++) {
        doc[String(i + 1)] = relayState[i] ? "on" : "off";
    }
    String json;
    serializeJson(doc, json);
    Serial.println(json);
    mqttManager.publish(status_topic, json.c_str(), true);
}

// ===== MQTT callback =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    Serial.printf("[MQTT] RX | %s : %s\n", topic, msg.c_str());

    if (String(topic) == command_topic) {
        for (int i = 0; i < totalPins; i++) {

            // Ignore only if switch JUST changed
            if (switchChangedAfterBoot[i]) {
                Serial.printf("[PRIORITY] Relay %d ignored web (switch override active)\n", i+1);
                continue;
            }

            if (msg == String(i+1) + "on") {
                relayState[i] = HIGH;
            } else if (msg == String(i+1) + "off") {
                relayState[i] = LOW;
            } else {
                continue;
            }

            digitalWrite(outputPins[i], relayState[i]);
            prefs.putBool(("relay" + String(i)).c_str(), relayState[i]);

            Serial.printf("[WEB] Relay %d set to %s\n", i+1,
                          relayState[i] ? "ON" : "OFF");
        }

        publishAllStates();
    }
}

// ===== Setup =====
void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println("\n[BOOT] ESP32 starting...");

    for (int i = 0; i < totalPins; i++) {
        pinMode(outputPins[i], OUTPUT);
        pinMode(inputPins[i], INPUT);
    }

    prefs.begin("relay_store");

    // 1️⃣ Restore relay states from EEPROM
    for (int i = 0; i < totalPins; i++) {
        relayState[i] = prefs.getBool(("relay" + String(i)).c_str(), LOW);
        digitalWrite(outputPins[i], relayState[i]);
        Serial.printf("[EEPROM] Relay %d restored: %s\n",
                      i+1, relayState[i] ? "ON" : "OFF");
    }

    // 2️⃣ Read switches and override if different
    for (int i = 0; i < totalPins; i++) {
        bool sw = digitalRead(inputPins[i]);
        lastSwitchState[i] = sw;

        if (sw != relayState[i]) {
            relayState[i] = sw;
            digitalWrite(outputPins[i], sw);
            prefs.putBool(("relay" + String(i)).c_str(), sw);
            switchChangedAfterBoot[i] = true;

            Serial.printf("[SWITCH] Relay %d overridden at boot\n", i+1);
        }
    }

    Serial.println("[WIFI] Connecting...");
    wifiManager.begin();
    Serial.println("[WIFI] Connected");

    mqttManager.setCallback(mqttCallback);
    mqttManager.begin();
}

// ===== Loop =====
void loop() {
    mqttManager.loop();

    // 🔑 IMPORTANT FIX IS HERE
    static bool firstSyncDone = false;
    if (mqttManager.isConnected() && !firstSyncDone) {
        firstSyncDone = true;

        Serial.println("[SYNC] Initial sync done → web control enabled");

        // 🔥 CLEAR SWITCH OVERRIDE AFTER BOOT
        for (int i = 0; i < totalPins; i++) {
            switchChangedAfterBoot[i] = false;
        }

        publishAllStates();
    }

    // Switch polling
    static unsigned long lastPoll = 0;
    if (millis() - lastPoll > 100) {
        lastPoll = millis();

        for (int i = 0; i < totalPins; i++) {
            bool sw = digitalRead(inputPins[i]);

            if (sw != lastSwitchState[i]) {
                lastSwitchState[i] = sw;
                switchChangedAfterBoot[i] = true;

                relayState[i] = sw;
                digitalWrite(outputPins[i], sw);
                prefs.putBool(("relay" + String(i)).c_str(), sw);

                Serial.printf("[SWITCH] Relay %d changed to %s\n",
                              i+1, sw ? "ON" : "OFF");

                publishAllStates();
            }
        }
    }
}
