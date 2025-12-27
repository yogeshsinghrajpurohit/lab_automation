#include "mqtt_manager.h"

MQTTManager::MQTTManager(const char* s, int p, const char* u, const char* pw)
    : server(s), port(p), user(u), password(pw), client(espClient) {}

void MQTTManager::setCallback(std::function<void(char*, byte*, unsigned int)> cb) {
    callback = cb;
    client.setCallback([this](char* t, byte* p, unsigned int l) {
        callback(t, p, l);
    });
}

void MQTTManager::begin() {
    client.setServer(server, port);
    Serial.println("[MQTT] Broker configured");
}

void MQTTManager::loop() {
    if (!client.connected()) {

        Serial.println("[MQTT] Connecting to broker...");
        String clientId = "ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);

        if (client.connect(
                clientId.c_str(),
                user,
                password,
                "dev/online",
                1,
                true,
                "offline"
            )) {

            Serial.println("[MQTT] Connected");
            client.subscribe("dev/command");
            Serial.println("[MQTT] Subscribed: dev/command");

            client.publish("dev/online", "online", true);
            Serial.println("[MQTT] Published: dev/online = online");
        } else {
            Serial.printf("[MQTT] Failed, rc=%d\n", client.state());
            delay(3000);
        }
    }

    client.loop();
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retain) {
    Serial.printf("[MQTT] Publish → %s : %s\n", topic, payload);
    return client.publish(topic, payload, retain);
}

bool MQTTManager::isConnected() {
    return client.connected();
}
