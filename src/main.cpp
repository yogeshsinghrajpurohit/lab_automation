#include "wifi_manager.h"
#include "mqtt_manager.h"
#include <Preferences.h>
#include <ArduinoJson.h>

const char* ssid = "Innovation_Lab";
const char* password = "Mlp0Zaq1";

const char* mqtt_server = "192.168.10.41";
const int mqtt_port = 1883;
const char* mqtt_user = "engineer";
const char* mqtt_password = "123456";

const char* status_topic  = "dev/status";
const char* command_topic = "dev/command";

WiFiManager wifiManager(ssid, password);
MQTTManager mqttManager(mqtt_server, mqtt_port, mqtt_user, mqtt_password);
Preferences prefs;

int outputPins[] = {23,22,21,19,18,17,16,4};
int inputPins[]  = {36,39,34,35,32,33,25,26};
const int totalPins = 8;

bool relayState[8];
bool lastSwitchState[8];
bool switchChangedAfterBoot[8] = {false};

// ===== Publish relay states =====
void publishAllStates() {
    JsonDocument doc;
    for (int i = 0; i < totalPins; i++) {
        doc[String(i+1)] = relayState[i] ? "on" : "off";
    }
    String json;
    serializeJson(doc, json);
    mqttManager.publish(status_topic, json.c_str(), true);
}

// ===== MQTT callback =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

    if (String(topic) == command_topic) {
        for (int i = 0; i < totalPins; i++) {

            // Physical switch has priority
            if (switchChangedAfterBoot[i]) continue;

            if (msg == String(i+1) + "on") {
                relayState[i] = HIGH;
            }
            else if (msg == String(i+1) + "off") {
                relayState[i] = LOW;
            }

            digitalWrite(outputPins[i], relayState[i]);
            prefs.putBool(("relay"+String(i)).c_str(), relayState[i]);
        }
        publishAllStates();
    }
}

void setup() {
    Serial.begin(115200);

    for (int i = 0; i < totalPins; i++) {
        pinMode(outputPins[i], OUTPUT);
        pinMode(inputPins[i], INPUT);
    }

    prefs.begin("relay_store");

    // 1️⃣ Restore EEPROM relay states
    for (int i = 0; i < totalPins; i++) {
        relayState[i] = prefs.getBool(("relay"+String(i)).c_str(), LOW);
        digitalWrite(outputPins[i], relayState[i]);
    }

    // 2️⃣ Read physical switches and override if changed
    for (int i = 0; i < totalPins; i++) {
        bool sw = digitalRead(inputPins[i]);
        lastSwitchState[i] = sw;

        if (sw != relayState[i]) {
            relayState[i] = sw;
            digitalWrite(outputPins[i], sw);
            prefs.putBool(("relay"+String(i)).c_str(), sw);
            switchChangedAfterBoot[i] = true;
        }
    }

    wifiManager.begin();
    mqttManager.setCallback(mqttCallback);
    mqttManager.begin();
}

void loop() {
    mqttManager.loop();

    static bool firstSyncDone = false;
    if (mqttManager.isConnected() && !firstSyncDone) {
        firstSyncDone = true;
        publishAllStates();
    }

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
                prefs.putBool(("relay"+String(i)).c_str(), sw);

                publishAllStates();
            }
        }
    }
}
